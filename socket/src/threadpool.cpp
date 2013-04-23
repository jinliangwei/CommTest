
#include "tcp_comm.hpp"
#include "threadpool.hpp"
#include <pthread.h>
#include <new>
#include <string.h>
#include <sys/select.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

#define RDBUFSIZE 256

namespace commtest {

  //throw -1 if error with malloc
  tcpthreadpool::tcpthreadpool(asize_t _num_thrs, asize_t _max_conns, dsize_t _buff_size, callback _urcbk):
    num_thrs(_num_thrs),
    max_conns(_max_conns),
    buff_size(_buff_size),
    sockaddrs(new sockaddr_in[_max_conns]),
    wthrs(new pthread_t[_num_thrs]),
    rthrs(new pthread_t[_num_thrs]),
    wthrinfo(new tcpthrinfo_t[_num_thrs]),
    rthrinfo(new tcpthrinfo_t[_num_thrs]),
    rthrpipe(new int[_num_thrs]),
    rdthrbuf(new boost::scoped_array<uint8_t> *[_num_thrs])
  {     
    asize_t conn_per_thr = max_conns/num_thrs;
    asize_t conn_rem = max_conns%num_thrs;
    int i = 0;
    for(i = 0; i < num_thrs; ++i){
      if(i < conn_rem){
        wthrinfo[i].init_socks(conn_per_thr + 1);
        rthrinfo[i].init_socks(conn_per_thr + 1);
      }else{
        wthrinfo[i].init_socks(conn_per_thr);
        rthrinfo[i].init_socks(conn_per_thr); 
      }
      rthrinfo[i].set_thrid(i + num_thrs);
      wthrinfo[i].set_thrid(i);
      rthrinfo[i].cbk = _urcbk;
      rdthrbuf[i] = new boost::scoped_array<uint8_t>(new uint8_t[buff_size]);
      wthrinfo[i].set_buff(buff_size, NULL);
      rthrinfo[i].set_buff(buff_size, rdthrbuf[i]->get());
    }

    for(i = 0; i < num_thrs; ++i){
      int fd[2];
      if(pipe(fd) < 0){
        std::bad_alloc ex;
        throw ex;
      }
      LOG(DBG, stderr, "read pipe %d for thr %d\n", fd[0], i + num_thrs);
      rthrinfo[i].set_rdfd(fd[0]);
      rthrpipe[i] = fd[1];
    }

    running = false;
  }

  tcpthreadpool::~tcpthreadpool(){
  }

  int tcpthreadpool::add_conn(int sock, const sockaddr_in *clientaddr){
    if(running) return -1; 
    if(num_conns == max_conns) return -1;

    LOG(DBG, stderr, "add connection %p\n", clientaddr);
    thrid_t thrid = num_conns%num_thrs;
    wthrinfo[thrid].add_conn(sock);

    rthrinfo[thrid].add_conn(sock);

    memcpy((sockaddrs.get() + num_conns), clientaddr, sizeof(sockaddr_in));
    return num_conns++;
  }

  void *tcpthreadpool::start_send(void *info){
    while(1){
      tcpthrinfo_t *thrinfo = (tcpthrinfo_t *) info;
      msg_t *msg = thrinfo->msgq.pop();

      switch(msg->type){
        case EXIT:
          delete msg;
          return NULL;
        case WRITE:
          if(msg->sock < 0){
            asize_t idx;
            for(idx = 0; idx < thrinfo->num_socks; idx++){
              LOG(DBG, stderr, "broadcast to all conns of thr, msglen = %d\n", msg->len);
              LOG(DBG, stderr, "broadcast to all conns of thr, msg = %c%c\n", msg->data[4], msg->data[5]);
              writen(thrinfo->socks[idx], msg->data.get(), msg->len);
            }
          }else{
            writen(thrinfo->socks[msg->sock], msg->data.get(), msg->len);
          }

          delete msg;
          break;
        default:
          assert(false);
      }
    }
  }

  void *tcpthreadpool::start_read(void *info){
    tcpthrinfo_t *thrinfo = (tcpthrinfo_t *) info;
    cbk_data_t cbk_data;
    uint8_t *rdbuf = thrinfo->rdbuf;
    dsize_t bufsize = thrinfo->bufsize;
    uint8_t *tempbuf;
    bool use_tempbuf;
    while(1){

      fd_set rset;
      asize_t idx;
      int readyfd, rsize;

      while(1){
        FD_ZERO(&rset);
        for(idx = 0; idx < thrinfo->num_socks; idx++){
          FD_SET(thrinfo->socks[idx], &rset);
        }
        FD_SET(thrinfo->rdfd, &rset);

        LOG(DBG, stderr, "rthr %u start select()\n", thrinfo->thrid);

        readyfd = select(thrinfo->max_sockfd + 1, &rset, NULL, NULL, NULL);
        assert(readyfd > 0);
        LOG(DBG, stderr, "rthr %u has stuff to read, readyfd = %d\n", thrinfo->thrid, readyfd);

        for(idx = 0; idx < thrinfo->num_socks; idx++){
          if(FD_ISSET(thrinfo->socks[idx], &rset)){   

            LOG(DBG, stderr, "rthr %u read from %d\n", thrinfo->thrid, thrinfo->socks[idx]);
            rsize = tcp_read(thrinfo->socks[idx], rdbuf, bufsize, &tempbuf, &use_tempbuf);
            if(rsize == 0){
              //TODO: add a call back for exited client?
              LOG(DBG, stderr, "client exited\n");
              continue;
            }
            assert(rsize > 0);
            if(rsize > 0){
              cbk_data.len = rsize;
              cbk_data.data = (use_tempbuf) ? tempbuf : rdbuf;
              thrinfo->cbk(&cbk_data);
              if(use_tempbuf) free_tempbuf(tempbuf);
            }
            //TODO: what if read less than 0? deal with error!
          }
        }
        if(FD_ISSET(thrinfo->rdfd, &rset)){
          rsize = read(thrinfo->rdfd, rdbuf, bufsize);
          assert(rsize >= 0);
          //TODO: check for EOF and error
          if(rsize == 0){ // something written
            LOG(DBG, stderr, "rthr %u read from %d, exiting!!\n", thrinfo->thrid, thrinfo->rdfd);
            for(idx = 0; idx < thrinfo->num_socks; idx++){
              close(thrinfo->socks[idx]);
            }
            close(thrinfo->rdfd);
            return NULL;
          }
          //TODO: deal with pause and other cases
        }
      }
    }
  }

  int tcpthreadpool::start_all(){
    if(running) return -1;
    asize_t i;
    int ret;
    for(i = 0; i < num_thrs; i++){
      ret = pthread_create(&wthrs[i], NULL, start_send, &wthrinfo[i]);
      if(ret < 0) return -1;
      ret = pthread_create(&rthrs[i], NULL, start_read, &rthrinfo[i]);
      if(ret < 0) return -1;
    }
    running = true;
    return 0;
  }

  int tcpthreadpool::thr_broadcast(thrid_t thrid, uint8_t *msg, dsize_t len){
    if(!running) return -1;
    if(thrid >= num_thrs) return -2;
    msg_t *m = new msg_t(WRITE, msg, len, -1);
    wthrinfo[thrid].msgq.push(m);
    return 0;
  }

  int tcpthreadpool::send(cliid_t cliid, uint8_t *msg, dsize_t len){
    if(!running) return -1;
    if(cliid >= num_conns) return -2;

    asize_t thrid = cliid%num_thrs;
    asize_t sockidx = cliid/num_thrs;
    msg_t *m = new msg_t(WRITE, msg, len, sockidx);
    wthrinfo[thrid].msgq.push(m);
    return 0;
  }

  int tcpthreadpool::broadcast(uint8_t *msg, dsize_t len){
    if(!running) return -1;

    asize_t i;
    for(i = 0; i < num_thrs; ++i){
      msg_t *m = new msg_t(WRITE, msg, len, -1);
      wthrinfo[i].msgq.push(m);
    }
    return 0;
  }

  int tcpthreadpool::stop_all(){
    if(!running) return -1;
    asize_t i;
    void *status;
    int ret;
    for(i = 0; i < num_thrs; ++i){
      msg_t *m = new msg_t(EXIT, NULL, 0, -1);
      wthrinfo[i].msgq.push(m);
      //writen(rthrpipe[i], (uint8_t *) "a", 1);
      close(rthrpipe[i]);
    }

    for(i = 0; i < num_thrs; ++i){
      ret = pthread_join(wthrs[i], &status);
      LOG(DBG, stderr, "wthr[%d] joined\n", i);
      assert(ret == 0);
      ret = pthread_join(rthrs[i], &status);
      LOG(DBG, stderr, "rthr[%d] joined\n", i);
      assert(ret == 0);
    }
    return 0;
  }

}
