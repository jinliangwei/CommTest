#ifndef __COMM_HANDLER_HPP__
#define __COMM_HANDLER_HPP__

#include <boost/scoped_ptr.hpp>
#include <zmq.hpp>
#include <pthread.h>
#include <string>
#include <stdint.h>
#include <queue>
#include <semaphore.h>

#define DBG 0  // print when debugging
#define ERR 1  // print when error should be exposed
#define NOR 2  // always print
#define MSGQ_ENDP "inproc://msgq"
#define TASKQ_ENDP "inproc://taskq"
#define SHUTDOWN_ENDP "inproc://shutdown"

#ifndef COMM_HANDLER_LOGLEVEL
#define COMM_HANDLER_LOGLEVEL 0
#endif

#define LOG(LEVEL, OUT, FMT, ARGS...) if(LEVEL >= COMM_HANDLER_LOGLEVEL) fprintf(OUT, "[%s][%s][ln:%d]  "FMT, __FILE__, __FUNCTION__, __LINE__, ##ARGS)


namespace commtest{
  
  typedef int32_t cliid_t;

  template<class T>
  class pcqueue{ //producer-consumer queue
  private:
    std::queue<T> q;
    pthread_mutex_t mutex;
    sem_t sem;
  public:
    pcqueue(){
      pthread_mutex_init(&mutex, NULL);
      sem_init(&sem, 0, 0);
    }

    ~pcqueue(){
      pthread_mutex_destroy(&mutex);
      sem_destroy(&sem);
    }

    void push(T t){
      pthread_mutex_lock(&mutex);
      q.push(t);
      pthread_mutex_unlock(&mutex);
      sem_post(&sem);
    }

    T pop(){
      sem_wait(&sem); //semaphore lets no more threads in than queue size
      pthread_mutex_lock(&mutex);
      T t = q.front();
      q.pop();
      pthread_mutex_unlock(&mutex);
      return t;
    }
  };

  struct config_param_t{
    cliid_t id; //must be set; others are optional
    bool accept_conns; //default false
    std::string ip;
    std::string port;

    config_param_t():
      id(0),
      accept_conns(false),
      ip("localhost"),
      port("9000"){}

    config_param_t(cliid_t _id, bool _accept_conns, std::string _ip, std::string _port):
      id(_id),
      accept_conns(_accept_conns),
      ip(_ip),
      port(_port){}
  };
  
  class comm_handler {
  private:
    boost::scoped_ptr<zmq::context_t> zmq_ctx;
    boost::scoped_ptr<zmq::socket_t> msgq; // a pull socket; application threads push to a corresponding socket
    boost::scoped_ptr<zmq::socket_t> taskq; // a push socket; application threads pull from a corresponding socket

    //shared among worker threads, should be protected by mutex
    boost::scoped_ptr<zmq::socket_t> msgpush;
    boost::scoped_ptr<zmq::socket_t> taskpull;

    boost::scoped_ptr<zmq::socket_t> router_sock;
    boost::scoped_ptr<zmq::socket_t> monitor_sock;

    boost::scoped_ptr<zmq::socket_t> shutdown_sock; // a pull socket; if read anything, then shut down
    
    pthread_mutex_t msgq_mtx;
    pthread_mutex_t taskq_mtx;
    pthread_mutex_t sync_mtx;
    pthread_t pthr;
    cliid_t id;
    std::string ip;
    std::string port;
    int errcode; // 0 is no error
    pcqueue<cliid_t> clientq;
    bool accept_conns;
    static void *start_handler(void *_comm);
  public:
    
    comm_handler(config_param_t &cparam):
      zmq_ctx(new zmq::context_t(1)),
      msgq(new zmq::socket_t(*(zmq_ctx.get()), ZMQ_PULL)),
      taskq(new zmq::socket_t(*(zmq_ctx.get()), ZMQ_PUSH)),
      msgpush(new zmq::socket_t(*(zmq_ctx.get()), ZMQ_PUSH)),
      taskpull(new zmq::socket_t(*(zmq_ctx.get()), ZMQ_PULL)),
      router_sock(new zmq::socket_t(*(zmq_ctx.get()), ZMQ_ROUTER)),
      monitor_sock(new zmq::socket_t(*(zmq_ctx.get()), ZMQ_PAIR)),
      shutdown_sock(new zmq::socket_t(*(zmq_ctx.get()), ZMQ_PULL)),
      id(cparam.id),
      ip(cparam.ip),
      port(cparam.port),
      accept_conns(cparam.accept_conns)
    {
      pthread_mutex_init(&msgq_mtx, NULL);
      pthread_mutex_init(&taskq_mtx, NULL);
      pthread_mutex_init(&sync_mtx, NULL);
      
      try{	
	msgq->bind(MSGQ_ENDP);
	msgpush->connect(MSGQ_ENDP);
	taskq->bind(TASKQ_ENDP);      
	taskpull->connect(TASKQ_ENDP);
	shutdown_sock->bind(SHUTDOWN_ENDP);
      }catch(zmq::error_t e){
	LOG(DBG, stderr, "Failed to connect to inproc socket: %s\n", e.what());
	throw std::bad_alloc();
      }
      LOG(DBG, stderr, "inproc socket connection established\n");
      pthread_mutex_lock(&sync_mtx);
    
      int ret = pthread_create(&pthr, NULL, start_handler, this);
      if(ret != 0) throw std::bad_alloc();
      pthread_mutex_lock(&sync_mtx); //cannot return until comm thread starts running
      errcode = 0;
    }
    
    ~comm_handler(){
      pthread_mutex_destroy(&msgq_mtx);
      pthread_mutex_destroy(&taskq_mtx);
    }

    
    cliid_t wait_for_connection();
    int connect_to(std::string ip, std::string port);
    
    int broadcast(uint8_t *data, size_t len);
    int send(cliid_t cid, uint8_t *data, size_t len); //non-blocking send
    int recv(cliid_t *cid, uint8_t **data); //blocking recv; client needs to clear data
    int shutdown();
  };


}
#endif
