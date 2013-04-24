#include "comm_handler.hpp"
#include "util.hpp"

int loglevel = 0;

namespace commtest {

  int comm_handler::send(cliid_t cid, uint8_t *data, size_t len){
    pthread_mutex_lock(&msgq_mtx);
    int ret = send_msg(*msgq, cid, data, len, 0);
    pthread_mutex_unlock(&msgq_mtx);

    return ret;
  }

  int comm_handler::recv(cliid_t *cid, uint8_t **data){
    pthread_mutex_lock(&taskq_mtx);
    int size = recv_msg(*taskq, data, len, cid);
    pthread_mutex_unlock(&taskq_mtx);

    return size;
  }

  void *comm_hadler::start_listen(void *_comm){
    comm_handler *comm = _comm;

    zmq::pollitem_t pollitems[3];
    pollitems[0].socket = comm->msgq;
    pollitems[0].events = ZMQ_POLLIN;
    //pollitems[1].socket = comm->taskq;
    //pollitems[1].events = ZMQ_POLLIN;
    pollitems[2].socket = comm->shutdown;
    pollitems[2].events = ZMQ_POLLIN;
    while(true){
      LOG(DBG, stderr, "comm_handler starts listening...\n");

      try { 
	zmq::poll(pollitems, 3);
      } catch (...) {
	LOG(NOR, stderr, "zmq::poll error!\n");
	break;
      }

      
      if(pollitems[0].revents){
	LOG(DBG, stderr, "message on msgq\n");
	uint8_t *data;
	size_t len;
	bool more;
	more = recv_msg(comm->msgq, &data, &len);
	if(more){
	  LOG(DBG, stderr, "not yet support composite message! ignored!\n");
	  continue;
	}
	LOG(DBG, stderr, "got message from msg queue, len = %d\n", len);
	LOG(DBG, stderr, "msg = %s\n", (char *) data);
	//TODO send msg out
      }

      /* TODO if there's tasks coming in
      if(){

      } */

      if(pollitems[2].revents){
	LOG(DBG, stderr, "thread shuting down!\n");
	return;
      }

    }
  }
}
