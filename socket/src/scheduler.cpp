
#include <boost/program_options.hpp>
namespace boost_po = boost::program_options;

#include "threadpool.hpp"
#include "util.hpp"
#include "tcp_comm.hpp"
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <assert.h>
#include <semaphore.h>
int loglevel = 0;

using namespace commtest;
sem_t sem;

void *inc_cnt(void *cbk_data){
  
  LOG(DBG, stderr, "received message!\n");
  sem_post(&sem);
  return NULL;
}

int main(int argc, char *argv[]){
  boost_po::options_description options("Allowed options");
  std::string ip = "127.0.0.1";
  uint16_t port = 9999;
  asize_t nthrs = 2;
  asize_t max_conns = 2;
  asize_t nconns = 2;
  asize_t buffsize = 2048;
  int exp = 1;

  options.add_options()
    ("ip", boost_po::value<std::string>(&ip), "ip address")
    ("port", boost_po::value<asize_t>(&port), "port number")
    ("mconns", boost_po::value<asize_t>(&max_conns), "max number of connections")
    ("nthrs", boost_po::value<asize_t>(&nthrs), "number of threads")
    ("buffsize", boost_po::value<asize_t>(&buffsize), "size of buffer")
    ("log", boost_po::value<int>(&loglevel), "log level")
    ("exp", boost_po::value<int>(&exp), "experiment number");
  
  boost_po::variables_map options_map;
  boost_po::store(boost_po::parse_command_line(argc, argv, options), options_map);
  boost_po::notify(options_map);  
  
  if(exp != 1) return 0;

  int ssock = tcp_socket();
  assert(ssock >= 0);
  int success = tcp_bind(ssock, ip.c_str(), port);
  assert(success >= 0);
  int connsock;
  sockaddr_in clientaddr;

  sem_init(&sem, 0, 0);
  tcpthreadpool netmgr(nthrs, max_conns, buffsize, inc_cnt);

  for(nconns = 0; nconns < max_conns; ++nconns){
    success = tcp_listen(ssock);
    assert(success >= 0);
    
    connsock = tcp_accept(ssock, &clientaddr);
    assert(connsock >= 0);
    
    netmgr.add_conn(connsock, &clientaddr);
  }
  netmgr.start_all();
  char msg[] = "helloworld";
  netmgr.broadcast((uint8_t *) msg, strlen(msg));
  sem_wait(&sem);
  netmgr.stop_all();
}
