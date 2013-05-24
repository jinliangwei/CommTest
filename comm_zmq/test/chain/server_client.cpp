#include <boost/shared_array.hpp>
#include <boost/program_options.hpp>
namespace boost_po = boost::program_options;

#include <string>
#include <comm_handler.hpp>
#include <stdio.h>

using namespace commtest;


int main(int argc, char *argv[]){
  boost_po::options_description options("Allowed options");
  std::string ip;
  std::string port;
  cliid_t id; 
  std::string sip;
  std::string sport;
  cliid_t sid; 
  int num_clients;
  cliid_t *clients;
  
  options.add_options()
    ("id", boost_po::value<cliid_t>(&id)->default_value(0), "node id")
    ("ip", boost_po::value<std::string>(&ip)->default_value("127.0.0.1"), "ip address")
    ("port", boost_po::value<std::string>(&port)->default_value("9998"), "port number")
    ("sid", boost_po::value<cliid_t>(&sid)->default_value(0), "scheduler id")
    ("sip", boost_po::value<std::string>(&sip)->default_value("127.0.0.1"), "ip address")
    ("sport", boost_po::value<std::string>(&sport)->default_value("9999"), "port number")
    ("ncli", boost_po::value<int>(&num_clients)->default_value(1), "number of clients expected");
  
  boost_po::variables_map options_map;
  boost_po::store(boost_po::parse_command_line(argc, argv, options), options_map);
  boost_po::notify(options_map);  
  
  LOG(DBG, stderr, "server started at %s:%s\n", ip.c_str(), port.c_str());

  try{
    clients = new cliid_t[num_clients];
  }catch(std::bad_alloc e){
    LOG(NOR, stderr, "failed to allocate memory for client array\n");
    return -1;
  }
  
  config_param_t config(id, true, ip, port); 
  
  comm_handler *comm;
  try{
    comm = new comm_handler(config);
  }catch(std::bad_alloc e){
    LOG(NOR, stderr, "failed to create comm\n");
    return -1;
  }
  LOG(NOR, stderr, "comm_handler created\n");
  
  //get num_clients - 1 connections
  int i;
  for(i = 0; i < num_clients - 1; i++){
    cliid_t cid;
    cid = comm->get_one_connection();
    if(cid < 0){
      LOG(NOR, stderr, "wait for connection failed\n");
      return -1;
    }
    LOG(NOR, stderr, "received connection from %d\n", cid);
    clients[i] = cid;
  }
  LOG(NOR, stderr, "initiate connection!");
  int suc = comm->connect_to(sip, sport, sid);
  if(suc < 0) LOG(NOR, stderr, "failed to connect to server\n");

  boost::shared_array<uint8_t> data;
  cliid_t rid;

  suc = comm->recv(rid, data);
  assert(suc > 0 && rid == sid);

  printf("Received msg : %d from %d\n", *((cliid_t *) data.get()), sid);

  // get another client
  // if this part is removed, we can make a 
  // server<->server_client<->server_client<->...<->server_client<->client chain
  // just to play with
  cliid_t cid;
  cid = comm->get_one_connection();
  if(cid < 0){
    LOG(NOR, stderr, "wait for connection failed\n");
    return -1;
  }
  LOG(NOR, stderr, "received connection from %d\n", cid);
  clients[num_clients-1] = cid;


  for(i = 0; i < num_clients; ++i){
    suc = comm->send(clients[i], (uint8_t *) (clients + i), sizeof(cliid_t));
    
    LOG(NOR, stderr, "send task to %d\n", clients[i]);
    assert(suc == sizeof(cliid_t));
  }
  
  for(i = 0; i < num_clients; ++i){
    boost::shared_array<uint8_t> data; //I'm expecting a string
    cliid_t cid;
    suc = comm->recv(cid, data);
    assert(suc > 0);
    printf("Received msg : %s from %d\n", (char *) data.get(), cid);
  }

  suc = comm->send(sid, (uint8_t *) "hello", 6);
  assert(suc > 0);

  LOG(NOR, stdout, "TEST NEARLY PASSED!! SHUTTING DOWN COMMTHREAD!!\n");
  suc = comm->shutdown();
  if(suc < 0) LOG(NOR, stderr, "failed to shut down comm handler\n");

  delete comm;
  LOG(NOR, stdout, "TEST PASSED!! EXITING!!\n");
  return 0;
}
