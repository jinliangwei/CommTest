
#include <sys/socket.h>
#include <sys/types.h>
#include <strings.h>
#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include "threadpool.hpp"
#include "util.hpp"

#define BACKLOG (64) //default connection queue length

namespace commtest{
  // For all functions, nonnegative = OK, negative = error
  
  int tcp_socket(){
    return socket(AF_INET, SOCK_STREAM, 0);
  }

  int tcp_bind(int sock, const char *ip, uint16_t port){
    struct sockaddr_in tcpaddr;

    bzero(&tcpaddr, sizeof(sockaddr_in));
 
    tcpaddr.sin_family = AF_INET;
    tcpaddr.sin_port = htons(port);
    int success = inet_pton(AF_INET, ip, &(tcpaddr.sin_addr));

    LOG(DBG, stdout, "ip address string to number inet_pton return %d\n", success);
    if(success <= 0) return -1;
    
    return bind(sock, (sockaddr *) &tcpaddr, sizeof(sockaddr_in));
  }

  int tcp_listen(int sock){
    return listen(sock, BACKLOG);
  }

  int tcp_accept(int sock, sockaddr_in *clientaddr){
    socklen_t sock_len;
    char client_ip[INET_ADDRSTRLEN];
    int connsock = accept(sock, (sockaddr *) clientaddr, &sock_len);

    if(connsock < 0) return -1;
    assert(sock_len == sizeof(sockaddr_in)); // I only accept IPv4 connections

    const char *ptr = inet_ntop(AF_INET, &(clientaddr->sin_addr), client_ip, INET_ADDRSTRLEN);
    assert(ptr != NULL);

    LOG(DBG, stdout, "Accepted connectin from %s:%d\n", client_ip, ntohs(clientaddr->sin_port));
    
    return connsock;
  }

  int tcp_connect(int sock, const char *ip, uint16_t port){
    sockaddr_in tcpaddr;

    bzero(&tcpaddr, sizeof(sockaddr_in));
 
    tcpaddr.sin_family = AF_INET;
    tcpaddr.sin_port = htons(port);
    int success = inet_pton(AF_INET, ip, &(tcpaddr.sin_addr));
    LOG(DBG, stdout, "ip address string to number inet_pton return %d\n", success);
    if(success <= 0) return -1;
    
    return connect(sock, (sockaddr *) &tcpaddr, sizeof(sockaddr_in));
  }

  /* The three functions below are merely used for testing purpose */
  int tcp_read(int sock, uint8_t *buff, dsize_t maxsize, uint8_t **tempbuf, bool *use_tempbuf){
    uint32_t n;
    dsize_t msglen;
    n = readn(sock, (uint8_t *) &msglen, sizeof(dsize_t));
    if(n == 0){
      LOG(DBG, stderr, "client exited\n");
      return 0;
    }
    if(n < 0) return -1;
    
    LOG(DBG, stderr, "msglen = %u\n", msglen);

    if(msglen <= maxsize){
      n = readn(sock, buff, msglen);
      *use_tempbuf = false;
    }else{
      try{
	*tempbuf = new uint8_t[msglen + 1];
      }catch(std::bad_alloc e){
	LOG(DBG, stderr, "caught exception e");
	throw e;
      }
      n = readn(sock, *tempbuf, msglen);
      *use_tempbuf = true;
    }

    LOG(DBG, stdout, "received\n");
    return n;
  }

  void free_tempbuf(uint8_t *tempbuf){
    delete[] tempbuf;
  }

  int tcp_write(int sock, uint8_t *buff, dsize_t msglen){
    uint32_t n;
    n = writen(sock, (uint8_t *) &msglen, sizeof(dsize_t));
    if(n < 0) return -1;
    n = writen(sock, buff, msglen);
    
    return n;
  }
  
}
