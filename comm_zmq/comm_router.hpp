#ifndef __COMM_ROUTER__
#define __COMM_ROUTER__

#include <boost/make_shared.hpp>
#include <boost/bind.hpp> 
#include <vector>
#include <string>

using namespace std;

class CommRouter {
  private:

  shared_ptr<zmq::context_t> zmq_ctx;

  // inproc sockets 
  shared_ptr<zmq::socket_t> to_thr_sock;  // PUSH socket
  shared_ptr<zmq::socket_t> from_thr_sock;  // PULL socket
  
  // router socket.
  shared_ptr<zmq::socket_t> router_sock;  

  //vector<string> connect_to;
  //vector<string> bind_to;

  // background thread executing send and receive msg.
  shared_ptr<thread> handler_thread;

  public:

  CommRouter(
      shared_ptr<zmq::context_t> ctx 
      , string to_thr_sock_ip
      , string from_thr_sock_ip
      , vector<string> connect_to
      , vector<string> bind_to
      , string identity = ""  // optionally used when connect_to is specified.
      ) :
    zmq_ctx(ctx)
  {

    router_sock(*zmq_ctx, ZMQ_ROUTER);
    if (identity != "") {
      router_sock.setsockopt(ZMQ_IDENTITY, identity.c_str(), identity.size());
    }
    BOOST_FOREACH(string s, connect_to) {
      router_sock.connect(s.c_str()); 
    }
    BOOST_FOREACH(string s, bind_to) {
      router_sock.bind(s.c_str()); 
    }

    to_thr_sock(*zmq_ctx, ZMQ_PUSH);
    from_thr_sock(*zmq_ctx, ZMQ_PULL);

    handler_thread = make_shared<thread>(bind(&CommRouter::do_handler, this));
    sleep(1); // make sure do_handler is really running.
  }

  ~CommRouter() { };

  private:
  void do_handler();


}

#endif // __COMM_ROUTER__
