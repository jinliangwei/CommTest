#ifndef __COMMTEST_ZMQ_UTIL_HPP__
#define __COMMTEST_ZMQ_UTIL_HPP__

#include "comm_handler.hpp"
#include <zmq.hpp>

int recv_msg(zmq::socket_t& sock, uint8_t **data)
{
    zmq::message_t msgt;
    sock.recv(&msgt);

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

int recv_msg(zmq::socket_t& sock, uint8_t **data, cliid_t *cid)
{
    zmq::message_t msgt;
    sock.recv(&msgt);
    size_t len = msgt.size();
    assert(len == sizeof(cliid_t));
    *cid = *((cliid_t *) msgt.data());

    len = recv_msg(sock, data);
    if(len <= 0) return -1;

    return len;
}


int send_msg(zmq::socket_t &sock, uint8_t *data, size_t len, int flag){
  zmq::message_t msg(len);
  memcpy(msg.data(), data, len);
  
  return sock.send(msg, data, flag);
}

int send_msg(zmq::socket_t &sock, cliid_t cid, uint8_t *data, size_t len, int flag)
{
  
  int ret = send_msg(sock, &cid, sizeof(cliid_t), flag | zmq::ZMQ_SNDMORE);
  if(ret == sizeof(cliid_t)) return -1;
  return send_msg(sock, &cid, sizeof(cliid_t), flag);

}

#endif
