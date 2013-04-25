#ifndef __ZMQ_UTIL_HPP__
#define __ZMQ_UTIL_HPP__

#include "comm_handler.hpp"
#include <zmq.hpp>
#include <assert.h>

int recv_msg(zmq::socket_t *sock, uint8_t **data)
{
    zmq::message_t msgt;
    sock->recv(&msgt);

    size_t len = msgt.size();    
    try{
      *data = new uint8_t[len];
    }catch(std::bad_alloc e){
      LOG(DBG, stderr, "can not allocate memory!\n");
      return -1;
    }
    
    memcpy(*data, msgt.data(), len);
    return len;
}

int recv_msg(zmq::socket_t *sock, uint8_t **data, commtest::cliid_t *cid)
{
    zmq::message_t msgt;
    sock->recv(&msgt);
    size_t len = msgt.size();
    assert(len == sizeof(commtest::cliid_t));
    *cid = *((commtest::cliid_t *) msgt.data());

    len = recv_msg(sock, data);
    if(len <= 0) return -1;

    return len;
}


int send_msg(zmq::socket_t *sock, uint8_t *data, size_t len, int flag){
  
  int nbytes = sock->send(data, len, flag);
  LOG(DBG, stderr, "send out %d bytes, should %lu bytes\n", nbytes, len);
  return nbytes;
}

int send_msg(zmq::socket_t *sock, commtest::cliid_t cid, uint8_t *data, size_t len, int flag)
{
  
  int ret = send_msg(sock, (uint8_t *) &cid, sizeof(commtest::cliid_t), flag | ZMQ_SNDMORE);
  if(ret != sizeof(commtest::cliid_t)) return -1;
  return send_msg(sock, data, len, flag);

}

#endif
