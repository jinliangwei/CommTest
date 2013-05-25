#include <boost/shared_array.hpp>
#include <boost/program_options.hpp>
namespace boost_po = boost::program_options;

#include <string>
#include <comm_handler.hpp>
#include <stdio.h>

using namespace commtest;

struct thrinfo{
  int num_sthrs;
  int thrid;
  comm_handler *comm;
};

void *thr_recv(void *tin){
  thrinfo *info = (thrinfo *) tin;
  int i;
  boost::shared_array<uint8_t> data;
  for(i = 0; i < info->num_sthrs; ++i){
      cliid_t rid;
      int suc = info->comm->recv(rid, data);
      assert(suc > 0);
      printf("Thread %d, received msg : %s from %d\n", info->thrid, (char *) data.get(), rid);
  }
  return NULL;
}

int main(int argc, char *argv[]){
  boost_po::options_description options("Allowed options");
  std::string sip;
  std::string sport;
  cliid_t id; 
  cliid_t sid;
  int num_thrs;
  int num_sthrs;
  options.add_options()
    ("id", boost_po::value<cliid_t>(&id)->default_value(1), "node id")
    ("sid", boost_po::value<cliid_t>(&sid)->default_value(0), "scheduler id")
    ("sip", boost_po::value<std::string>(&sip)->default_value("127.0.0.1"), "ip address")
    ("sport", boost_po::value<std::string>(&sport)->default_value("9999"), "port number")
    ("nthr", boost_po::value<int>(&num_thrs)->default_value(2), "number of threads to use")
    ("nsthr", boost_po::value<int>(&num_sthrs)->default_value(2), "number of server threads");
  
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
  
  thrinfo *info = new thrinfo[num_thrs];
  pthread_t *thrs = new pthread_t[num_thrs];
  int i;
  for(i = 0; i < num_thrs; ++i){
    info[i].num_sthrs = num_sthrs;
    info[i].thrid = i;
    info[i].comm = comm;
  }
  LOG(NOR, stderr, "start threads to receive!\n");

  for(i = 0; i < num_thrs; ++i){
    pthread_create(&(thrs[i]), NULL, thr_recv, info + i);
  }

  for(i = 0; i < num_thrs; ++i){
    pthread_join(thrs[i], NULL);
  }

  suc = comm->send(sid, (uint8_t *) "hello", 6);
  assert(suc > 0);

  LOG(NOR, stdout, "TEST NEARLY PASSED!! SHUTTING DOWN COMM THREAD!!\n");
  suc = comm->shutdown();
  if(suc < 0) LOG(NOR, stderr, "failed to shut down comm handler\n");
  delete comm;
  LOG(NOR, stdout, "TEST PASSED!! EXITING!!\n");
  return 0;
}
