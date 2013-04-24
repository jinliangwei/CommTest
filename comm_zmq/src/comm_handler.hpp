#ifndef __COMM_HANDLER_HPP__
#define __COMM_HANDLER_HPP__

#include <boost/scoped_ptr.hpp>
#include <zmq.hpp>
#include <pthread.h>
#include <string>
#include <unistd.h>


#define DBG 0  // print when debugging
#define ERR 1  // print when error should be exposed
#define NOR 2  // always print

extern uint8_t loglevel; // user needs to define

#define LOG(LEVEL, OUT, FMT, ARGS...) if(LEVEL >= loglevel) fprintf(OUT, "[%s][%s][ln:%d]  "FMT, __FILE__, __FUNCTION__, __LINE__, ##ARGS)


namespace commtest{
  
  typedef uint16_t cliid_t;

  struct config_param_t{
    cliid_t identity;
    uint8_t loglevel;
  };
  
  const char *MSGQ_ENDP = "inproc://msgq";
  const char *TASKQ_ENDP = "inproc://taskq";

  class comm_handler {
  private:
    boost::scoped_ptr<zmq::context_t> zmqctx;
    boost::scoped_ptr<zmq::socket_t> msgq; // a pull socket; application threads push to a corresponding socket
    boost::scoped_ptr<zmq::socket_t> taskq; // a push socket; application threads pull from a corresponding socket

    //shared among worker threads, should be protected by mutex
    boost::scoped_ptr<zmq::socket_t> msgpush;
    boost::scoped_ptr<zmq::socket_t> taskpull;

    boost::scoped_ptr<zmq::socket_t> shutdown; // a pull socket; if read anything, then shut down
    
    pthread_mutex_t msgq_mtx;
    pthread_mutex_t taskq_mtx;
    pthread_t pthr;
    cliid_t identity;
    
    void *start_listen(void *_comm);
  public:
    
    comm_handler(config_param_t &cparam):
      zmqctx(new zmq::context_t(1)),
      msgq(new zmq::socket_t(zmq_ctx, zmq::ZMQ_PULL)),
      taskq(new zmq::socket_t(zmq_ctx, zmq::ZMQ_PUSH)),
      msgpush(new zmq::socket(zmq_ctx, zmq::ZMQ_PUSH)),
      taskpull(new zmq::socket(zmq_ctx, zmq::ZMQ_PULL)),
      shutdown(new zmq::socket(zmq_ctx, zmq::ZMQ_PULL)),
      msgq_mtx = PTHREAD_MUTEX_INITIALIZER,
      taskq_mtx = PTHREAD_MUTEX_INITIALIZER,
      identity(cparam.identity)
    {
      msgq->connect(MSGQ_ENDP);
      taskq->connect(TASKQ_ENDP);
      msgpush->connect(MSGQ_ENDP);
      taskpull->connect(TASKQ_ENDP);
      int ret = pthread_create(&pthr, NULL, start_listen, this);
      
      if(ret != 0) throw std::bad_alloc();
      loglevel = (cparam.loglevel > NOR) ? 0 : cparam.loglevel;
    }

    /*
      int wait_for_connection();
      int connect_to(string ip, uint16_t port);
    */
    
    int broadcast(uint8_t *data, size_t len);
    int send(cliid_t cid, uint8_t *data, size_t len); //non-blocking send
    int recv(cliid_t cid, uint8_t *data); //blocking recv; client needs to clear data
    int shutdown();
  };
}
#endif
