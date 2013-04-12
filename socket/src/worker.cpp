
#include <boost/program_options.hpp>
namespace boost_po = boost::program_options;

#include "util.hpp"
#include "tcp_comm.hpp"
#include <string>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <assert.h>

int loglevel = 0;
extern struct sockaddr_in clientaddr;

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

  LOG(DBG, stderr, "connection established\n");

  uint8_t buff[20];
  uint8_t *tempbuff;
  bool use_tempbuf;

  int rsize = tcp_read(ssock, buff, 20, &tempbuff, &use_tempbuf);
  assert(rsize > 0);
  buff[rsize] = '\0';
  LOG(DBG, stderr, "received %s\n", buff);
  tcp_write(ssock, buff, (dsize_t) rsize);

  close(ssock);

}
