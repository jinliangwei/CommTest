#include <boost/shared_array.hpp>
#include <boost/program_options.hpp>
namespace boost_po = boost::program_options;

#include <string>
#include <comm_handler.hpp>
#include <stdio.h>

using namespace commtest;

int main(int argc, char *argv[]){
  boost_po::options_description options("Allowed options");
  std::string sip;
  std::string sport;
  cliid_t id; 
  cliid_t sid;

  options.add_options()
    ("id", boost_po::value<cliid_t>(&id)->default_value(1), "node id")
    ("sid", boost_po::value<cliid_t>(&sid)->default_value(0), "scheduler id")
    ("sip", boost_po::value<std::string>(&sip)->default_value("127.0.0.1"), "ip address")
    ("sport", boost_po::value<std::string>(&sport)->default_value("9999"), "port number");
    
  
  boost_po::variables_map options_map;
  boost_po::store(boost_po::parse_command_line(argc, argv, options), options_map);
  boost_po::notify(options_map);  
  
  config_param_t config(id, false, "", ""); 
  
  comm_handler *comm;
  try{
    comm = new comm_handler(config);
  }catch(...){
    LOG(NOR, stderr, "failed to create comm\n");
    return -1;
  }
  
  int suc = comm->connect_to(sip, sport, sid);
  if(suc < 0) LOG(NOR, stderr, "failed to connect to server\n");

  boost::shared_array<uint8_t> data;
  cliid_t rid;

  suc = comm->recv(rid, data);
  assert(suc > 0 && rid == sid);

  printf("Received msg : %d from %d\n", *((cliid_t *) data.get()), sid);
  
  sleep(5);
  suc = comm->send(sid, (uint8_t *) "hello", 6);
  assert(suc > 0);

  LOG(NOR, stdout, "TEST NEARLY PASSED!! SHUTTING DOWN COMM THREAD!!\n");
  suc = comm->shutdown();
  if(suc < 0) LOG(NOR, stderr, "failed to shut down comm handler\n");
  delete comm;
  LOG(NOR, stdout, "TEST PASSED!! EXITING!!\n");
  return 0;
}
