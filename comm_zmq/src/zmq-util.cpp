
#include "zmq-util.hpp"

/*
 * return number of bytes received, negative if error 
 */
int recv_msg(zmq::socket_t *sock, uint8_t **data)
{
    zmq::message_t msgt;
    int nbytes;
    try{
      nbytes = sock->recv(&msgt);
    }catch(zmq::error_t e){
      //LOG(NOR, stderr, "recv failed: %s\n", e.what());
      return -1;
    }

    if(nbytes == 0) return 0;

    size_t len = msgt.size();    
    try{
      *data = new uint8_t[len];
    }catch(std::bad_alloc e){
      //LOG(DBG, stderr, "can not allocate memory!\n");
      return -1;
    }
    
    memcpy(*data, msgt.data(), len);
    return len;
}

/*
 * return number of bytes received, negative if error
 */
int recv_msg(zmq::socket_t *sock, uint8_t **data, commtest::cliid_t &cid)
{
    zmq::message_t msgt;
    try{
      sock->recv(&msgt);
    }catch(zmq::error_t e){
      //LOG(NOR, stderr, "recv cid failed: %s\n", e.what());
      return -1;
    }

    size_t len = msgt.size();
    if(len != sizeof(commtest::cliid_t)) return -1;

    cid = *((commtest::cliid_t *) msgt.data());
    //LOG(DBG, stderr, "received id = 0x%x\n", cid);

    return recv_msg(sock, data);
}

/*
 * return number of bytes sent, negative if error
 */
int send_msg(zmq::socket_t *sock, uint8_t *data, size_t len, int flag){
  
  int nbytes;
  try{
    nbytes = sock->send(data, len, flag);
  }catch(zmq::error_t e){
    //LOG(NOR, stderr, "send failed: %s\n", e.what());
    return -1;
  }
  return nbytes;
}

int send_msg(zmq::socket_t *sock, commtest::cliid_t cid, uint8_t *data, size_t len, int flag)
{
  
  //LOG(DBG, stderr, "send to cid = 0x%x\n", cid);
  int nbytes;
  try{    
    nbytes = send_msg(sock, (uint8_t *) &cid, sizeof(commtest::cliid_t), flag | ZMQ_SNDMORE);
  }catch(zmq::error_t e){
    //LOG(NOR, stderr, "send cid failed: %s\n", e.what());
    return -1;
  }

  if(nbytes != sizeof(commtest::cliid_t)){
    //LOG(NOR, stderr, "send client id failed, sent %d bytes\n", nbytes); 
    return -1;
  }
  return send_msg(sock, data, len, flag);
}
