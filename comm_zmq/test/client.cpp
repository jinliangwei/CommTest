
#include <boost/program_options.hpp>
namespace boost_po = boost::program_options;

#include <string>
#include <comm_handler.hpp>
#include <stdio.h>

using namespace commtest;

/*
 * A simple TCP echo server
 */

int main(int argc, char *argv[]){
  boost_po::options_description options("Allowed options");
  std::string sip;
  std::string sport;
  cliid_t id; 

  options.add_options()
    ("id", boost_po::value<cliid_t>(&id)->default_value(2), "node id")
    ("ip", boost_po::value<std::string>(&sip)->default_value("127.0.0.1"), "ip address")
    ("port", boost_po::value<std::string>(&sport)->default_value("9999"), "port number");  

  boost_po::variables_map options_map;
  boost_po::store(boost_po::parse_command_line(argc, argv, options), options_map);
  boost_po::notify(options_map);  
  
  config_param_t config(id, false, sip, sport); 
  
  comm_handler *comm;
  try{
    comm = new comm_handler(config);
  }catch(...){
    LOG(NOR, stderr, "failed to create comm\n");
    return -1;
  }
  
  int suc = comm->connect_to(sip, sport);
  if(suc < 0) LOG(NOR, stderr, "failed to connect to server\n");
  while(1);

  uint8_t *data;
  cliid_t sid;

  //suc = comm->send(1, (uint8_t *) &id, sizeof(uint8_t));
  //assert(suc >= 0);

  suc = comm->recv(&sid, &data);
  assert(suc > 0);
  printf("Received msg : %d from %d\n", (cliid_t) *data, sid);
  delete[] data;

  suc = comm->send(sid, (uint8_t *) "hello", 6);
  assert(suc > 0);

  suc = comm->shutdown();
  if(suc < 0) LOG(NOR, stderr, "failed to shut down comm handler\n");

  delete comm;
  return 0;
}
