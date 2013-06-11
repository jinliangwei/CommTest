#include "comm_handler.hpp"
#include <assert.h>
#include "zmq_util.hpp"
#include <unistd.h>

#define COMM_HANDLER_INIT "hello"

//TODO, check return value for functions which may cause error

namespace commtest {

  cliid_t comm_handler::get_one_connection(){
    if(!accept_conns) return -1;
    if(errcode) return -1;
    return clientq.pop();
  }

  int comm_handler::connect_to(std::string destip, std::string destport, cliid_t destid){
    if(errcode) return -1;    
    std::string connstr = std::string("tcp://") + destip + std::string(":") + destport;


    if(connpush.get() == NULL){
      zmq::socket_t *sock;
      try{
        sock = new zmq::socket_t(zmq_ctx, ZMQ_PUSH);
      }catch(std::bad_alloc &ba){
        return -1;
      }
      connpush.reset(sock);
      try{
        connpush->connect(CONN_ENDP);
      }catch(zmq::error_t &e){
        LOG(DBG, stderr, "Failed to connect to inproc socket: %s\n", e.what());
        return -1;
      }
    }

    pthread_mutex_lock(&sync_mtx);
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

  int comm_handler::init_multicast(config_param_t &cparam){
    LOG(DBG, stderr, "init multicast\n");
    // create multicast groups if needed
    if(cparam.mc_tocreate.size() > 0){
      max_pub = cparam.mc_tocreate.size();
      mc_pub.reset(new boost::shared_ptr<zmq::socket_t>[max_pub]);
      int idx;
      for(idx = 0; idx < max_pub; ++idx){
        std::string connstr = std::string("epgm://")
                          + cparam.mc_tocreate[idx].ip + std::string(";")
                          + cparam.mc_tocreate[idx].multicast_addr
                          + std::string(":")
                          + cparam.mc_tocreate[idx].multicast_port;
        LOG(DBG, stderr, "mulitcast bind to %s\n", connstr.c_str());
        try{
          mc_pub[idx].reset(new zmq::socket_t(zmq_ctx, ZMQ_PUB));
          mc_pub[idx]->bind(connstr.c_str());
          mc_pub[idx]->setsockopt(ZMQ_RATE, &cparam.multicast_rate, sizeof(cparam.multicast_rate));
        }catch (...){
          errcode = 1;
          return -1;
        }
      }
    }

    if(cparam.mc_tojoin.size() > 0){
      try{
        mc_sub.reset(new zmq::socket_t(zmq_ctx, ZMQ_SUB));
      }catch(...){
        errcode = 1;
        return -1;
      }

      recv_mc = true;

      int idx;
      for(idx = 0; idx < (int) cparam.mc_tojoin.size(); ++idx){
        std::string connstr = std::string("epgm://")
                          + cparam.mc_tocreate[idx].ip + std::string(";")
                          + cparam.mc_tocreate[idx].multicast_addr
                          + std::string(":")
                          + cparam.mc_tocreate[idx].multicast_port;
       try{
         LOG(DBG, stderr, "mulitcast connect to %s\n", connstr.c_str());
          mc_sub->connect(connstr.c_str());
          mc_sub->setsockopt(ZMQ_RATE, &cparam.multicast_rate, sizeof(cparam.multicast_rate));
        }catch (zmq::error_t &e){
          errcode = 1;
          return -1;
        }
      }
    }
    return 0;
  }

  int comm_handler::mc_send(cliid_t mc_group, uint8_t *data, size_t len){
    if(errcode) return -1;

    if(mc_sendpush.get() == NULL){
      zmq::socket_t *sock;
      try{
        sock = new zmq::socket_t(zmq_ctx, ZMQ_PUSH);
      }catch(std::bad_alloc &ba){
        return -1;
      }
      mc_sendpush.reset(sock);
      try{
        mc_sendpush->connect(MC_SEND_ENDP);
      }catch(zmq::error_t &e){
        return -1;
      }
    }

    int ret = send_msg(*mc_sendpush, mc_group, data, len, 0);
    if(errcode) return -1;
    return ret;
  }

  int comm_handler::send(cliid_t cid, uint8_t *data, size_t len){
    if(errcode) return -1;

    if(msgpush.get() == NULL){
      zmq::socket_t *sock;
      try{
        sock = new zmq::socket_t(zmq_ctx, ZMQ_PUSH);
      }catch(std::bad_alloc &ba){
        return -1;
      }
      msgpush.reset(sock);
      try{
        msgpush->connect(MSGQ_ENDP);
      }catch(zmq::error_t &e){
        return -1;
      }
    }

    int ret = send_msg(*msgpush, IDE2I(cid), data, len, 0);
    if(errcode) return -1;
    return ret;
  }

  int comm_handler::recv(cliid_t &cid, boost::shared_array<uint8_t> &data){
    if(errcode) return -1;
    LOG(DBG, stderr, "recv task!!!!!\n");    
    if(taskpull.get() == NULL){
      zmq::socket_t *sock;
      try{
        sock = new zmq::socket_t(zmq_ctx, ZMQ_PULL);
      }catch(std::bad_alloc &ba){
        return -1;
      }
      taskpull.reset(sock);
      try{
        taskpull->connect(TASKQ_ENDP);
      }catch(zmq::error_t &e){
        return -1;
      }
    }

    int incid;
    int size = recv_msg(*taskpull, incid, data);
    cid = IDI2E(incid);
    if(errcode) return -1;
    return size;
  }

  int comm_handler::recv_async(cliid_t &cid, boost::shared_array<uint8_t> &data){
    if(errcode) return -1;
    LOG(DBG, stderr, "recv_async task!!!!!\n");    
    if(taskpull.get() == NULL){
      zmq::socket_t *sock;
      try{
        sock = new zmq::socket_t(zmq_ctx, ZMQ_PULL);
      }catch(std::bad_alloc &ba){
        return -1;
      }
      taskpull.reset(sock);
      try{
        taskpull->connect(TASKQ_ENDP);
      }catch(zmq::error_t &e){
        return -1;
      }
    }

    int incid;
    int size = recv_msg_async(*taskpull, incid, data);
    if(size == 0) return 0;
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
    }catch(zmq::error_t &e){
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
    }catch(zmq::error_t &e){
      LOG(DBG, stderr, "router_sock monitor connection failed\n");
      pthread_mutex_unlock(&(comm->sync_mtx));
      return NULL;
    }
    
    if(comm->accept_conns){
      std::string conn_str = std::string("tcp://") + comm->ip + std::string(":") + comm->port;
      LOG(DBG, stderr, "router_sock tries to bind to %s\n", conn_str.c_str());	
      try{
        comm->router_sock.bind(conn_str.c_str());
      }catch(zmq::error_t &e){
        LOG(DBG, stderr, "router_sock binds to %s failed: %s\n", conn_str.c_str(), e.what());
        pthread_mutex_unlock(&(comm->sync_mtx));
        return NULL;
      }
    }

    pthread_mutex_unlock(&(comm->sync_mtx));

    zmq::pollitem_t pollitems[7];
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
    pollitems[5].socket = comm->mc_sendpull;
    pollitems[5].events = ZMQ_POLLIN;
    if(comm->recv_mc){
      pollitems[6].socket = *comm->mc_sub;
      pollitems[6].events = ZMQ_POLLIN;
    }

    while(true){
      LOG(DBG, stderr, "comm_handler starts listening...\n");

      try { 
        int num_poll;
        if(comm->recv_mc){
          num_poll = zmq::poll(pollitems, 7);
        }else{
          num_poll = zmq::poll(pollitems, 6);
        }
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

        len = recv_msg(comm->msgq, cid, data);
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
	
        len = recv_msg(comm->router_sock, cid, data);
        if(len < 0){
          LOG(DBG, stderr, "recv from router_sock failed");
          comm->errcode = 1;
          return NULL;
        }
	
        LOG(DBG, stderr, "got task from router socket, identitylen = %d\n", len);

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
        int ret = recv_msg(comm->conn_sock, destid, data);
        if(ret < 0){
          LOG(DBG, stderr, "recv from conn_sock failed!\n");
          comm->errcode = 1;
          pthread_mutex_unlock(&(comm->sync_mtx));
          return NULL;
        }
	
        connstr_c = (char *) data.get();
        try{
          comm->router_sock.connect(connstr_c);
        }catch(zmq::error_t &e){
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

      if(pollitems[5].revents){
        LOG(DBG, stderr, "message on mc_sendpull\n");
        boost::shared_array<uint8_t> data;
        int len;
        cliid_t gid;

        len = recv_msg(comm->mc_sendpull, gid, data);

        if(len < 0){
          LOG(DBG, stderr, "recv from mc_sendpull failed");
          comm->errcode = 1;
          return NULL;
        }
        if(gid < 0 || gid >= comm->max_pub){
          comm->errcode = 1;
          return NULL;
        }
        send_msg(*comm->mc_pub[gid], data.get(), len, 0);
      }

      if(comm->recv_mc && pollitems[6].revents){
        boost::shared_array<uint8_t> data;
        int len;

        len = recv_msg(*comm->mc_sub, data);
        if(len < 0){
          LOG(DBG, stderr, "recv from mc_sub failed");
          comm->errcode = 1;
          return NULL;
        }
        send_msg(comm->taskq, -1, data.get(), len, 0);
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
