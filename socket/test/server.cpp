
#include <boost/program_options.hpp>
namespace boost_po = boost::program_options;

#include <src/util.hpp>
#include <src/tcp_comm.hpp>
#include <string>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <assert.h>
#include <netinet/in.h>

int loglevel = 0;

sockaddr_in clientaddr;

#define BUFSIZE 30

uint8_t buff[BUFSIZE];
uint8_t *tempbuf;
bool use_tempbuf;
using namespace commtest;
/*
 * A simple TCP echo server
 */

int main(int argc, char *argv[]){
  boost_po::options_description options("Allowed options");
  std::string ip = "127.0.0.1";
  uint16_t port = 9999;
  
  options.add_options()
    ("ip", boost_po::value<std::string>(&ip), "ip address")
    ("port", boost_po::value<uint16_t>(&port), "port number")
    ("log", boost_po::value<int>(&loglevel), "log level");
  
  boost_po::variables_map options_map;
  boost_po::store(boost_po::parse_command_line(argc, argv, options), options_map);
  boost_po::notify(options_map);  
  
  printf("server started at %s:%d\n", ip.c_str(), port);

  int ssock = tcp_socket();
  
  assert(ssock >= 0);

  int success = tcp_bind(ssock, ip.c_str(), port);
  assert(success >= 0);

  success = tcp_listen(ssock);
  assert(success >= 0);

  int connsock = tcp_accept(ssock, &clientaddr);
  assert(connsock >= 0);
  close(ssock); // accept only one connection

  int msglen = tcp_read(connsock, buff, BUFSIZE - 1, &tempbuf, &use_tempbuf);
  assert(msglen > 0);
  buff[msglen] = '\0';
  LOG(DBG, stderr, "received: %s\n", buff);
  tcp_write(connsock, buff, msglen);
  
  close(connsock);
}
