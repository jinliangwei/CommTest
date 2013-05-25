#include <boost/shared_array.hpp>
#include <boost/program_options.hpp>
namespace boost_po = boost::program_options;

#include <string>
#include <comm_handler.hpp>
#include <stdio.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

using namespace commtest;

struct thrinfo{
  int num_clis;
  int num_cthrs;
  int thrid;
  cliid_t *clients;
  comm_handler *comm;
};

void *thr_send(void *tin){
  thrinfo *info = (thrinfo *) tin;
  int i, j;
  char buff[30];
  for(i = 0; i < info->num_clis; ++i){
    int suc = -1;
    for(j = 0; j < info->num_cthrs; ++j){
      sprintf(buff, "%d-%d-%d", info->thrid, i, j);
      suc = info->comm->send(info->clients[i], (uint8_t *) buff, strlen(buff) + 1);
      LOG(NOR, stderr, "send task %s to %d\n", buff, info->clients[i]);
    }
    assert(suc == (strlen(buff) + 1));
  }
  return NULL;
}

int main(int argc, char *argv[]){
  boost_po::options_description options("Allowed options");
  std::string ip;
  std::string port;
  cliid_t id; 
  int num_clients;
  cliid_t *clients;
  int num_thrs;
  int num_cthrs;
 
  options.add_options()
    ("id", boost_po::value<cliid_t>(&id)->default_value(0), "node id")
    ("ip", boost_po::value<std::string>(&ip)->default_value("127.0.0.1"), "ip address")
    ("port", boost_po::value<std::string>(&port)->default_value("9999"), "port number")
    ("ncli", boost_po::value<int>(&num_clients)->default_value(1), "number of clients expected")
    ("nthr", boost_po::value<int>(&num_thrs)->default_value(2), "number of threads to use")
    ("ncthr", boost_po::value<int>(&num_cthrs)->default_value(2), "number of threads on each client");
  
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
  
  int i;
  for(i = 0; i < num_clients; i++){
    cliid_t cid;
    cid = comm->get_one_connection();
    if(cid < 0){
      LOG(NOR, stderr, "wait for connection failed\n");
      return -1;
    }
    LOG(NOR, stderr, "received connection from %d\n", cid);
    clients[i] = cid;
  }
  
  LOG(NOR, stderr, "received expected number of clients, send tasks out!\n");
  thrinfo *info = new thrinfo[num_thrs];
  pthread_t *thrs = new pthread_t[num_thrs];
  for(i = 0; i < num_thrs; ++i){
    info[i].num_clis = num_clients;
    info[i].clients = clients;
    info[i].num_cthrs = num_cthrs;
    info[i].thrid = i;
    info[i].comm = comm;
  }

  for(i = 0; i < num_thrs; ++i){
    pthread_create(&(thrs[i]), NULL, thr_send, info + i);
  }

  for(i = 0; i < num_thrs; ++i){
    pthread_join(thrs[i], NULL);
  }

  LOG(NOR, stderr, "send all");

  for(i = 0; i < num_clients; ++i){
    boost::shared_array<uint8_t> data; //I'm expecting a string
    cliid_t cid;
    int suc = comm->recv(cid, data);
    assert(suc > 0);
    printf("Received msg : %s from %d\n", (char *) data.get(), cid);
  }

  LOG(NOR, stdout, "TEST NEARLY PASSED!! SHUTTING DOWN COMMTHREAD!!\n");
  int suc = comm->shutdown();
  if(suc < 0) LOG(NOR, stderr, "failed to shut down comm handler\n");

  delete comm;
  LOG(NOR, stdout, "TEST PASSED!! EXITING!!\n");
  return 0;
}
