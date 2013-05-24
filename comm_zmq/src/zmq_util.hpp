#ifndef __ZMQ_UTIL_HPP__
#define __ZMQ_UTIL_HPP__

#include "comm_handler.hpp"
#include <zmq.hpp>
#include <assert.h>
#include <boost/shared_array.hpp>
/*
 * return number of bytes received, negative if error 
 */
int recv_msg(zmq::socket_t *sock, boost::shared_array<uint8_t> &data);

/*
 * return number of bytes received, negative if error
 */
int recv_msg(zmq::socket_t *sock, boost::shared_array<uint8_t> &data, commtest::cliid_t &cid);

/*
 * return number of bytes sent, negative if error
 */
int send_msg(zmq::socket_t *sock, uint8_t *data, size_t len, int flag);

int send_msg(zmq::socket_t *sock, commtest::cliid_t cid, uint8_t *data, size_t len, int flag);

#endif
