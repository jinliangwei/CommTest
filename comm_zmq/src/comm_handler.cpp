#include "comm_handler.hpp"
#include <assert.h>
#include "zmq_util.hpp"
#include <unistd.h>

#define COMM_HANDLER_INIT "hello"

namespace commtest {

  cliid_t comm_handler::get_one_connection(){
    if(!accept_conns) return -1;
    if(errcode) return -1;
    return clientq.pop();
  }

  int comm_handler::connect_to(std::string destip, std::string destport, cliid_t destid){
    if(errcode) return -1;    
    std::string connstr = std::string("tcp://") + destip + std::string(":") + destport;
    pthread_mutex_lock(&sync_mtx);

    if(connpush.get() == NULL){
      zmq::socket_t *sock;
      try{
	sock = new zmq::socket_t(zmq_ctx, ZMQ_PUSH);
      }catch(std::bad_alloc ba){
	pthread_mutex_unlock(&sync_mtx);
	return -1;
      }
      connpush.reset(sock);
      try{
	connpush->connect(CONN_ENDP);
      }catch(zmq::error_t e){
	LOG(DBG, stderr, "Failed to connect to inproc socket: %s\n", e.what());
	return -1;
      }
    }

    int ret = send_msg(*connpush, IDE2I(destid), (uint8_t *) connstr.c_str(), connstr.size(), 0);
    if(ret <= 0){
      pthread_mutex_unlock(&sync_mtx);
      return -1;
    }
    pthread_mutex_lock(&sync_mtx); //cannot return until comm thread starts running
    pthread_mutex_unlock(&sync_mtx);
    if(errcode) return -1;
    return 0;
  }

  int comm_handler::broadcast(uint8_t *data, size_t len){
    //not yet supported
    assert(0);
    return -1;
  }

  int comm_handler::send(cliid_t cid, uint8_t *data, size_t len){
    if(errcode) return -1;

    if(msgpush.get() == NULL){
      zmq::socket_t *sock;
      try{
	sock = new zmq::socket_t(zmq_ctx, ZMQ_PUSH);
      }catch(std::bad_alloc ba){
	return -1;
      }
      msgpush.reset(sock);
      try{
	msgpush->connect(MSGQ_ENDP);
      }catch(zmq::error_t e){
	return -1;
      }
    }

    int ret = send_msg(*msgpush, IDE2I(cid), data, len, 0);
    if(errcode) return -1;
    return ret;
  }

  int comm_handler::recv(cliid_t &cid, boost::shared_array<uint8_t> &data){
    LOG(DBG, stderr, "recv task before check!!!!!\n");
    if(errcode) return -1;
    LOG(DBG, stderr, "recv task!!!!!\n");
    
    if(taskpull.get() == NULL){
      zmq::socket_t *sock;
      try{
	sock = new zmq::socket_t(zmq_ctx, ZMQ_PULL);
      }catch(std::bad_alloc ba){
	return -1;
      }
      taskpull.reset(sock);
      try{
	taskpull->connect(TASKQ_ENDP);
      }catch(zmq::error_t e){
	return -1;
      }
    }

    int incid;
    int size = recv_msg(*taskpull, data, incid);
    cid = IDI2E(incid);
    if(errcode) return -1;
    return size;
  }

  //static function
  void *comm_handler::start_handler(void *_comm){
    comm_handler *comm = (comm_handler *) _comm;
    
    try{
      int sock_mandatory = 1;
      comm->router_sock.setsockopt(ZMQ_IDENTITY, &(comm->id), sizeof(cliid_t));
      comm->router_sock.setsockopt(ZMQ_ROUTER_MANDATORY, &(sock_mandatory), sizeof(int));
      LOG(DBG, stderr, "set my router sock identity to 0x%x\n", comm->id);
    }catch(zmq::error_t e){
      LOG(DBG, stderr, "router_sock->setsockopt failed\n");
      pthread_mutex_unlock(&(comm->sync_mtx));
      return NULL;
    }

    //add more events to monitor if needed
    int ret = zmq_socket_monitor(comm->router_sock, "inproc://monitor.router_sock", ZMQ_EVENT_CONNECTED | ZMQ_EVENT_ACCEPTED | ZMQ_EVENT_DISCONNECTED);
    if(ret < 0){
      LOG(DBG, stderr, "monitor router_sock failed\n");
      comm->errcode = 1;
      pthread_mutex_unlock(&(comm->sync_mtx));
      return NULL;
    }

    try{
      comm->monitor_sock.connect("inproc://monitor.router_sock");
    }catch(zmq::error_t e){
      LOG(DBG, stderr, "router_sock monitor connection failed\n");
      pthread_mutex_unlock(&(comm->sync_mtx));
      return NULL;
    }
    
    if(comm->accept_conns){
      std::string conn_str = std::string("tcp://") + comm->ip + std::string(":") + comm->port;
      LOG(DBG, stderr, "router_sock tries to bind to %s\n", conn_str.c_str());	
      try{
	comm->router_sock.bind(conn_str.c_str());
      }catch(zmq::error_t e){
	LOG(DBG, stderr, "router_sock binds to %s failed: %s\n", conn_str.c_str(), e.what());
	pthread_mutex_unlock(&(comm->sync_mtx));
	return NULL;
      }
    }

    pthread_mutex_unlock(&(comm->sync_mtx));

    zmq::pollitem_t pollitems[5];
    pollitems[0].socket = comm->msgq;
    pollitems[0].events = ZMQ_POLLIN;
    pollitems[1].socket = comm->router_sock;
    pollitems[1].events = ZMQ_POLLIN;
    pollitems[2].socket = comm->shutdown_sock;
    pollitems[2].events = ZMQ_POLLIN;
    pollitems[3].socket = comm->monitor_sock;
    pollitems[3].events = ZMQ_POLLIN;
    pollitems[4].socket = comm->conn_sock;
    pollitems[4].events = ZMQ_POLLIN;

    while(true){
      LOG(DBG, stderr, "comm_handler starts listening...\n");
      try { 
	int num_poll = zmq::poll(pollitems, 5);
	LOG(DBG, stderr, "num_poll = %d\n", num_poll);
      } catch (...) {
	comm->errcode = 1;
	LOG(NOR, stderr, "zmq::poll error!\n");
	break;
      }

      if(pollitems[0].revents){
	LOG(DBG, stderr, "message on msgq\n");
	boost::shared_array<uint8_t> data;
	int len;
	cliid_t cid;

	len = recv_msg(comm->msgq, data, cid);
	if(len < 0){
	  LOG(DBG, stderr, "recv from msgq failed");
	  comm->errcode = 1;
	  return NULL;
	}
	LOG(DBG, stderr, "got message from msg queue, len = %d\n", len);
	
	send_msg(comm->router_sock, cid, data.get(), len, 0);
      }
      
      if(pollitems[1].revents){
	LOG(DBG, stderr, "task received on router socket!\n");
	boost::shared_array<uint8_t> data;
	int len;
	cliid_t cid;
	
	len = recv_msg(comm->router_sock, data, cid);
	if(len < 0){
	  LOG(DBG, stderr, "recv from router_sock failed");
	  comm->errcode = 1;
	  return NULL;
	}
	
	LOG(DBG, stderr, "got task from router socket, len = %d\n", len);

	if(comm->clientmap.count(cid) == 0){
	  LOG(DBG, stderr, "this is a new client\n");
	  comm->clientmap[cid] = true;
	  comm->clientq.push(IDI2E(cid));
	}else if(comm->clientmap[cid] == false){
	  //TODO: no connection loss detection
	  LOG(DBG, stderr, "this is a reconnected client\n");
	  comm->clientmap[cid] = true;
	  comm->clientq.push(IDI2E(cid));
	}else{
	  LOG(DBG, stderr, "send task to taskq\n");
	  send_msg(comm->taskq, cid, data.get(), len, 0);
	}
      }
      
      if(pollitems[3].revents){
	zmq_event_t *event;
	boost::shared_array<uint8_t> data;
	int len;
	len = recv_msg(comm->monitor_sock, data);
	
	if (len < 0){
	  LOG(DBG, stderr, "recv from monitor_sock failed");
	  comm->errcode = 1;
	  return NULL;
	}
	assert(len == sizeof(zmq_event_t));
	event = (zmq_event_t *) data.get();

	switch (event->event){
	case ZMQ_EVENT_CONNECTED:
	  LOG(DBG, stderr, "established connection.\n");
	  break;
	case ZMQ_EVENT_ACCEPTED:
	  LOG(DBG, stderr, "accepted connection.\n");
	  break;
	case ZMQ_EVENT_DISCONNECTED:
	  LOG(DBG, stderr, "client disconnected\n");
	  break;
	default:
	  LOG(DBG, stderr, "unexpected event : %d\n", event->event);
	  comm->errcode = 1;
	  return NULL;
	}
      }

      if(pollitems[4].revents){
	
	cliid_t destid;
	char *connstr_c;
	boost::shared_array<uint8_t> data;
	int ret = recv_msg(comm->conn_sock, data, destid);
	if(ret < 0){
	  LOG(DBG, stderr, "recv from conn_sock failed!\n");
	  comm->errcode = 1;
	  pthread_mutex_unlock(&(comm->sync_mtx));
	  return NULL;
	}
	
	connstr_c = (char *) data.get();
	try{
	  comm->router_sock.connect(connstr_c);
	}catch(zmq::error_t e){
	  LOG(DBG, stderr, "Connect failed : %s\n", e.what());
	  comm->errcode = 1;
	  pthread_mutex_unlock(&(comm->sync_mtx));
	  return NULL;
	}

	comm->clientmap[destid] = true;
	ret = -1;
	while(ret < 0){
	  ret = send_msg(comm->router_sock, destid, (uint8_t *) COMM_HANDLER_INIT, strlen(COMM_HANDLER_INIT) + 1, 0);
	}
	pthread_mutex_unlock(&(comm->sync_mtx));
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
    zmq::socket_t s(zmq_ctx, ZMQ_PUSH);
    s.connect(SHUTDOWN_ENDP);
    int ret = send_msg(s, (uint8_t *) &msg, sizeof(int), 0);
    if(ret < 0) return -1;
    ret = pthread_join(pthr, NULL);
    return ret;
  }
}
