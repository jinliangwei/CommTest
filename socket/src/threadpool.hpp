#ifndef __COMMTEST_THREADPOOL_H__
#define __COMMTEST_THREADPOOL_H__

#include <pthread.h>
#include "util.hpp"
#include <boost/scoped_array.hpp>
#include <assert.h>
#include <netinet/in.h>
#include <string.h>

#define DEF_NUMTHRS 8
#define DEF_MAXCONNS 64
#define DEF_BUFFSIZE 2048

namespace commtest {

  //TODO: double check the data type consistency and compbality with standard library
  typedef uint32_t dsize_t;
  typedef uint16_t thrid_t;
  typedef uint16_t cliid_t;
  typedef uint16_t asize_t;
  typedef void *(*callback)(void *);

  struct cbk_data_t{
    dsize_t len;
    uint8_t *data;
    
    cbk_data_t(dsize_t l, uint8_t *d):
      len(l), data(d){};
    
    cbk_data_t():
      len(0), data(NULL){};
  };

  enum msg_type {EXIT, WRITE};

  struct msg_t{
    msg_type type;
    // a memory shared by the external code and tcpthreadpool object, so it cannot be scoped_array
    boost::scoped_array<uint8_t> data; // for WRITE, this is the data to write, which is a byte array; not used if EXIT or READ
    dsize_t len;
    int sock; // for WRITE, destination sock idx, send to all if negative; for READ, read this one for pause info
    
    msg_t():
      type(EXIT),
      data(NULL),
      len(0),
      sock(0){}

    msg_t(msg_type t, uint8_t *d, dsize_t l, int s):
      type(t),
      data(new uint8_t[l + sizeof(dsize_t)]),
      len(l + sizeof(dsize_t)),
      sock(s)
    {
      uint8_t *buff = data.get();
      *((dsize_t *) buff) = l;
      LOG(DBG, stderr, "set data, size = %u\n", l);
      LOG(DBG, stderr, "set data, set msg size = %u\n", *((dsize_t *) buff));
      memcpy(buff + sizeof(dsize_t), d, l);
      LOG(DBG, stderr, "set data, after copy data, msg size = %u\n", *((dsize_t *) buff));
    }
  };

  struct tcpthrinfo_t{
    thrid_t thrid;
    int rdfd; // if read from this fd, then special procedure for pause
    pcqueue<msg_t *> msgq;
    boost::scoped_array<int> socks;
    asize_t num_socks;
    asize_t max_socks;
    int max_sockfd;
    dsize_t bufsize;
    callback cbk;
    uint8_t *rdbuf;

    tcpthrinfo_t():
      thrid(0),
      rdfd(0),
      socks(NULL),
      num_socks(0),
      max_socks(0),
      max_sockfd(0),
      bufsize(0),
      cbk(NULL),
      rdbuf(NULL){}

    ~tcpthrinfo_t(){
    }

    void set_thrid(thrid_t _thrid){
      thrid = _thrid;
    }

    //can throw std::bad_alloc
    int init_socks(asize_t _max_socks){
      max_socks = _max_socks;
      socks.reset(new int[max_socks]);
      LOG(DBG, stderr, "allocated memory for socks at %p\n", socks.get());
      return 0;
    }
    
    void set_rdfd(int pipefd){
      rdfd = pipefd;
      if(rdfd > max_sockfd) max_sockfd = rdfd;
    }

    void set_buff(dsize_t _bufsize, uint8_t *_rdbuf){
      bufsize = _bufsize;
      rdbuf = _rdbuf;
    }

    void add_conn(int sock){
      LOG(DBG, stderr, "add connection sock = %d\n", sock);
      socks[num_socks] = sock;
      ++num_socks;
      if(sock > max_sockfd) max_sockfd = sock;
      assert(max_socks >= num_socks);
    }
  };

  // this class is not thread safe to outside
  class tcpthreadpool {
  private:
    asize_t num_thrs;
    asize_t max_conns;
    asize_t num_conns;
    dsize_t buff_size;
    bool running;
    
    boost::scoped_array<sockaddr_in> sockaddrs;
    boost::scoped_array<pthread_t> wthrs; // write thread used to send msg
    boost::scoped_array<pthread_t> rthrs; // read from sock
    
    /**
     * socks is shared with wthrs and rthrs
     * Policies:
     * 1) wthrs and rthrs only read
     * 2) only thread that has tcpthreadpool can write to socks, before doing so, must stop all threads
     *    by calling pause_all()
     */
       
    boost::scoped_array<tcpthrinfo_t> wthrinfo;
    boost::scoped_array<tcpthrinfo_t> rthrinfo;
    boost::scoped_array<int> rthrpipe;
    boost::scoped_array<boost::scoped_array<uint8_t> *> rdthrbuf;

    //TODO: Add two bit arrays to keep track of lost connections and next slot for live connections

  private:
    static void *start_send(void * thrinfo);
    static void *start_read(void *thrinfo);
  
  public:
    static int cliid2thrid(cliid_t cliid);

    tcpthreadpool(asize_t _num_thrs = DEF_NUMTHRS, asize_t _max_conns = DEF_MAXCONNS,  dsize_t _buff_size = DEF_BUFFSIZE, callback _urcbk = NULL);
    ~tcpthreadpool();
    
    // add one established connection, return the id of the connection
    int add_conn(int sock, const sockaddr_in *sockaddr);
    int start_all();

    int thr_broadcast(thrid_t thrid, uint8_t *msg, dsize_t len);
    int send(cliid_t cliid, uint8_t *msg, dsize_t len);
    int broadcast(uint8_t *msg, dsize_t len);
    int stop_all();
    //int wait_all();

    //TODO: the following functions should be added to deal with connection loss and re-connect
    //TODO: add register callback for dead connection
    //void lost_conn_cbk();
    
    //TODO: pause all threads , can be used to add new connections
    //void pause_all();
    //int resume_all();
    
 };

}

#endif
