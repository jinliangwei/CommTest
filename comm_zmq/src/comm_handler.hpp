#ifndef __COMM_HANDLER_HPP__
#define __COMM_HANDLER_HPP__

#include <boost/scoped_ptr.hpp>
#include <zmq.hpp>
#include <pthread.h>
#include <string>
#include <stdint.h>
#include <queue>
#include <semaphore.h>
#include <boost/unordered_map.hpp>
#include <boost/shared_array.hpp>
#include <boost/thread/tss.hpp>

#define DBG 0  // print when debugging
#define ERR 1  // print when error should be exposed
#define NOR 2  // always print

#define MSGQ_ENDP "inproc://msgq"
#define TASKQ_ENDP "inproc://taskq"
#define SHUTDOWN_ENDP "inproc://shutdown"
#define CONN_ENDP "inproc://conn"

#ifndef COMM_HANDLER_LOGLEVEL
#define COMM_HANDLER_LOGLEVEL 0
#endif

#define IDE2I(x) (x << 4 | 0x1)
#define IDI2E(x) (x >> 4)

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
  
  //TODO: add two callbacks 1) connection loss 2) error state
  //TODO: add funciton to clear error state
  
  class comm_handler {
  private:
    zmq::context_t zmq_ctx;
    // a pull socket; application threads push to a corresponding socket
    zmq::socket_t msgq;
    // a push socket; application threads pull from a corresponding socket
    zmq::socket_t taskq;
    zmq::socket_t conn_sock;
    zmq::socket_t shutdown_sock; // a pull socket; if read anything, then shut down
    zmq::socket_t router_sock;
    zmq::socket_t monitor_sock;

    //shared among worker threads, should be protected by mutex
    boost::thread_specific_ptr<zmq::socket_t> msgpush;
    boost::thread_specific_ptr<zmq::socket_t> taskpull;
    boost::thread_specific_ptr<zmq::socket_t> connpush;    

    pthread_mutex_t sync_mtx;
    pthread_t pthr;
    cliid_t id;
    std::string ip;
    std::string port;
    int errcode; // 0 is no error
    pcqueue<cliid_t> clientq;
    boost::unordered_map<cliid_t, bool> clientmap;
    bool accept_conns;
    static void *start_handler(void *_comm);
    
  public:
    
    comm_handler(config_param_t &cparam):
      zmq_ctx(1),
      msgq(zmq_ctx, ZMQ_PULL),
      taskq(zmq_ctx, ZMQ_PUSH),
      conn_sock(zmq_ctx, ZMQ_PULL),
      shutdown_sock(zmq_ctx, ZMQ_PULL),
      router_sock(zmq_ctx, ZMQ_ROUTER),
      monitor_sock(zmq_ctx, ZMQ_PAIR),
      id(IDE2I(cparam.id)),
      ip(cparam.ip),
      port(cparam.port),
      accept_conns(cparam.accept_conns)
    {
      pthread_mutex_init(&sync_mtx, NULL);
      try{	
	msgq.bind(MSGQ_ENDP);
	taskq.bind(TASKQ_ENDP);
	conn_sock.bind(CONN_ENDP);
	shutdown_sock.bind(SHUTDOWN_ENDP);

      }catch(zmq::error_t e){
	LOG(DBG, stderr, "Failed to bind to inproc socket: %s\n", e.what());
	throw std::bad_alloc();
      }

      pthread_mutex_lock(&sync_mtx);
      errcode = 0;
      int ret = pthread_create(&pthr, NULL, start_handler, this);
      if(ret != 0) throw std::bad_alloc();
      pthread_mutex_lock(&sync_mtx); //cannot return until comm thread starts running
      pthread_mutex_unlock(&sync_mtx);
      if(errcode) throw std::bad_alloc();
    }
    
    ~comm_handler(){
      //pthread_mutex_destroy(&msgq_mtx);
      //pthread_mutex_destroy(&taskq_mtx);
      pthread_mutex_destroy(&sync_mtx);
    }

    
    cliid_t get_one_connection();
    int connect_to(std::string destip, std::string destport, cliid_t destid);
    
    int broadcast(uint8_t *data, size_t len);
    //not thread-safe
    int send(cliid_t cid, uint8_t *data, size_t len); //non-blocking send
    int recv(cliid_t &cid, boost::shared_array<uint8_t> &data); //blocking recv
    int recv_async(cliid_t &cid, boost::shared_array<uint8_t> &data); // nonblocking recv
    int shutdown();
  };
}
#endif
