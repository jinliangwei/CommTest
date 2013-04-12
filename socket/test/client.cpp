
#include <boost/program_options.hpp>
namespace boost_po = boost::program_options;

#include <src/util.hpp>
#include <src/tcp_comm.hpp>
#include <string>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <assert.h>

int loglevel = 0;
extern struct sockaddr_in clientaddr;

#define BUFSIZE 30

uint8_t buff[BUFSIZE];
uint8_t *tempbuf;
bool use_tempbuf;

using namespace commtest;
/*
 * A simple TCP echo client
 */

int main(int argc, char *argv[]){
  boost_po::options_description options("Allowed options");
  std::string sip = "127.0.0.1";
  uint16_t sport = 9999;

  options.add_options()
    ("sip", boost_po::value<std::string>(&sip), "server ip address")
    ("sport", boost_po::value<uint16_t>(&sport), "server port number")
    ("log", boost_po::value<int>(&loglevel), "log level");
  
  boost_po::variables_map options_map;
  boost_po::store(boost_po::parse_command_line(argc, argv, options), options_map);
  boost_po::notify(options_map);  
  

  int ssock = tcp_socket();
  
  assert(ssock >= 0);

  int success;
   
  printf("client starts connecting to server at %s:%d\n", sip.c_str(), sport);
  success = tcp_connect(ssock, sip.c_str(), sport);
  assert(success >= 0);
  
  char msg[] = "helloworld";
  int msglen = strlen(msg);  
  memcpy(buff, msg, msglen);

  int write = tcp_write(ssock, buff, msglen);
  assert(write == msglen);
  LOG(DBG, stderr, "sent: %s\n", msg);

  int read = tcp_read(ssock, buff, BUFSIZE - 1, &tempbuf, &use_tempbuf);
  assert(msglen == read);
  buff[read] = '\0';

  LOG(DBG, stderr, "received: %s\n", buff);

  close(ssock);

}
