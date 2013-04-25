#include "comm_handler.hpp"
#include <assert.h>
#include "zmq-util.hpp"

namespace commtest {

  cliid_t comm_handler::wait_for_connection(){
    if(errcode) return -1;
    return clientq.pop();
  }

  int comm_handler::connect_to(std::string destip, std::string destport){
    try{
      router_sock->connect((std::string("tcp://") + destip + std::string(":") + destport).c_str());
    }catch(zmq::error_t e){
      return -1;
    }
    send_msg(router_sock.get(), id, (uint8_t *) "hello", 6, 0);
    send_msg(router_sock.get(), id, (uint8_t *) "hello", 6, 0);

    return 0;
  }

  int comm_handler::broadcast(uint8_t *data, size_t len){
    //not yet supported
    assert(0);
    return -1;
  }

  int comm_handler::send(cliid_t cid, uint8_t *data, size_t len){
    if(errcode) return -1;
    pthread_mutex_lock(&msgq_mtx);
    int ret = send_msg(msgq.get(), cid, data, len, 0);
    pthread_mutex_unlock(&msgq_mtx);
    if(errcode) return -1;
    return ret;
  }

  int comm_handler::recv(cliid_t *cid, uint8_t **data){
    if(errcode) return -1;
    pthread_mutex_lock(&taskq_mtx);
    int size = recv_msg(taskq.get(), data, cid);
    pthread_mutex_unlock(&taskq_mtx);
    if(errcode) return -1;
    return size;
  }

  //static function
  void *comm_handler::start_handler(void *_comm){
    comm_handler *comm = (comm_handler *) _comm;
    
    try{
      comm->router_sock->setsockopt(ZMQ_IDENTITY, &(comm->id), sizeof(cliid_t));
    }catch(zmq::error_t e){
      LOG(DBG, stderr, "router_sock->setsockopt failed\n");
      pthread_mutex_unlock(&(comm->sync_mtx));
      return NULL;
    }
    
    //add more events to monitor if needed
    int ret = zmq_socket_monitor(*(comm->router_sock), "inproc://monitor.router_sock", ZMQ_EVENT_ACCEPTED | ZMQ_EVENT_DISCONNECTED);
    if(ret < 0){
      LOG(DBG, stderr, "monitor router_sock failed\n");
      comm->errcode = 1;
      pthread_mutex_unlock(&(comm->sync_mtx));
      return NULL;
    }

    try{
      comm->monitor_sock->connect("inproc://monitor.router_sock");
    }catch(zmq::error_t e){
      LOG(DBG, stderr, "router_sock monitor connection failed\n");
      pthread_mutex_unlock(&(comm->sync_mtx));
      return NULL;
    }
    
    if(comm->accept_conns){
      std::string conn_str = std::string("tcp://") + comm->ip + std::string(":") + comm->port;
      LOG(DBG, stderr, "router_sock tries to bind to %s\n", conn_str.c_str());	
      try{
	comm->router_sock->bind(conn_str.c_str());
      }catch(zmq::error_t){
	LOG(DBG, stderr, "router_sock binds to %s failed\n", conn_str.c_str());
	pthread_mutex_unlock(&(comm->sync_mtx));
	return NULL;
      }
    }

    pthread_mutex_unlock(&(comm->sync_mtx));

    zmq::pollitem_t pollitems[4];
    pollitems[0].socket = *(comm->msgq);
    pollitems[0].events = ZMQ_POLLIN;
    pollitems[1].socket = *(comm->router_sock);
    pollitems[1].events = ZMQ_POLLIN;
    pollitems[2].socket = *(comm->shutdown_sock);
    pollitems[2].events = ZMQ_POLLIN;
    pollitems[3].socket = *(comm->monitor_sock);
    pollitems[3].events = ZMQ_POLLIN;

    while(true){
      LOG(DBG, stderr, "comm_handler starts listening...\n");
      try { 
	int num_poll = zmq::poll(pollitems, 4);
	LOG(DBG, stderr, "num_poll = %d\n", num_poll);
      } catch (...) {
	comm->errcode = 1;
	LOG(NOR, stderr, "zmq::poll error!\n");
	break;
      }

      if(pollitems[0].revents){
	LOG(DBG, stderr, "message on msgq\n");
	uint8_t *data;
	int len;
	cliid_t cid;

	len = recv_msg(comm->msgq.get(), &data, &cid);
	if(len < 0){
	  comm->errcode = 1;
	  return NULL;
	}
	LOG(DBG, stderr, "got message from msg queue, len = %d\n", len);
	
	send_msg(comm->router_sock.get(), cid, data, len, 0);
	delete[] data;
	continue;
      }
      
      if(pollitems[1].revents){
	LOG(DBG, stderr, "task received on router socket!\n");
	uint8_t *data;
	int len;
	cliid_t cid;
	
	len = recv_msg(comm->router_sock.get(), &data, &cid);
	if(len < 0){
	  comm->errcode = 1;
	  return NULL;
	}

	LOG(DBG, stderr, "got task from router socket, len = %d\n", len);
	send_msg(comm->taskq.get(), data, len, 0); 
	
	delete[] data;
	continue;
      }
      
      if(pollitems[3].revents){
	zmq_event_t *event;
	int len;
	cliid_t cid;
	len = recv_msg(comm->monitor_sock.get(), (uint8_t **) &event);
	
	if (len < 0){
	  comm->errcode = 1;
	  return NULL;
	}
	assert(len == sizeof(zmq_event_t));
	switch (event->event){
	case ZMQ_EVENT_ACCEPTED:
	  //memcpy(&cid, event->data.connected.addr, sizeof(cliid_t));
	  LOG(DBG, stderr, "accepted connection fd %d.\n", event->data.accepted.fd);
	  LOG(DBG, stderr, "accepted connection addr %s.\n", event->data.accepted.addr);
	  comm->clientq.push(cid);
	  break;
	case ZMQ_EVENT_DISCONNECTED:
	  memcpy(&cid, event->data.connected.addr, sizeof(cliid_t));
	  LOG(DBG, stderr, "client disconnected %d\n", cid);
	  break;
	default:
	  LOG(DBG, stderr, "unexpected event : %d\n", event->event);
	  comm->errcode = 1;
	  return NULL;
	}
	continue;
      }
      
      if(pollitems[2].revents){
	LOG(DBG, stderr, "thread shuting down!\n");
	return NULL;
      }
    }
    return NULL;
  }
  
  int comm_handler::shutdown(){
    int msg = 1;
    zmq::socket_t s(*zmq_ctx, ZMQ_PUSH);
    s.connect(SHUTDOWN_ENDP);
    int ret = send_msg(&s, (uint8_t *) &msg, sizeof(int), 0);
    if(ret < 0) return -1;
    ret = pthread_join(pthr, NULL);
    return ret;
  }
}
