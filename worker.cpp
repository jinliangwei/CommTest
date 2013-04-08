/*******************************************************
@author: Jin Kyu Kim (jinkyuk@cs.cmu.edu)

 This file contains Worker thread for communication test 
 between scheduler_server / client / workers

********************************************************/

//#define _GNU_SOURCE 
#include "options.hpp"
#include "getinput.hpp"
#include "scheduler.hpp"
#include "dynamic.hpp"
#include "worker.hpp"

#include <assert.h>
#include <boost/program_options.hpp>
#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>

using namespace std;

unsigned int parsemsg(msg *t, struct thrctx *ctx){
  unsigned int jobtype;
  jobtype = t->type;

  DGPRINT(ERR, "\t worker(%d:%d) receive task type %X\n",
	  ctx->rank, ctx->lid, jobtype);

  return jobtype;
}
void *worker(void *arg){
  msg *wmsg;
  struct thrctx *ctx = (struct thrctx *)arg;

  DGPRINT(OUT, "  worker: rank %d local-id %d gid %d\n", 
	  ctx->rank, ctx->lid, ctx->gid); 
  pthread_barrier_wait(&ctx->mctx->bar_all); 
  /* barrier for all workers and sched-comm */

  while(1){

    wmsg = worker_get_task(); 
    /* get task msg from worker thread 
       and scheduler-client thrd */
    if(wmsg != NULL){
      DGPRINT(DBG, "  worker(%d,%d) get msg_seq %d\n", 
	      ctx->rank, ctx->lid, wmsg->seq);

      /* switch case statment to call worker function */


      wmsg->status = MSGSTATUS_DONE;
      worker_putback_task(wmsg);
    }
  }
  return NULL;
}
