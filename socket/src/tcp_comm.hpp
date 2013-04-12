
#ifndef __COMMTEST_TCP_COMM_H__
#define __COMMTEST_TCP_COMM_H__

#include <sys/types.h>
#include <netinet/in.h>
#include "threadpool.hpp"

namespace commtest{

  int tcp_socket();

  int tcp_bind(int sock, const char *ip, uint16_t port);

  int tcp_listen(int sock);

  int tcp_accept(int sock, sockaddr_in *clientaddr);

  int tcp_read(int sock, uint8_t *buff, dsize_t maxsize, uint8_t **tempbuf, bool *use_tempbuf);

  int free_tempbuf(uint8_t *tempbuf);

  int tcp_write(int sock, uint8_t *buff, dsize_t msglen);

  int tcp_connect(int sock, const char *ip, uint16_t port);

  int set_buff(const char *data, uint32_t len);
}

#endif
