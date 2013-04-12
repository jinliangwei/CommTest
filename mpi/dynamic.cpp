/********************************************************

   @author: Jin Kyu Kim (jinkyuk@cs.cmu.edu)
   Pstar based Parallel Distributed Lasso 

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

static struct problem pbcfg;  /* problem context of the input data set */
static struct context mctx;   /* machine context of application */
static struct experiments expt;

static struct thrctx **thctx;   /* ctx for array of workers per machine  */
static struct schedctx *schedc; /* ctx for scheduler on client side */
static struct schedctx *scheds; /* ctx for scheduler on server side */

static void create_threadcxt(struct context *ctx, struct problem *cfg, int rank);
static void printcfg(struct problem *cfg, struct context *mctx, struct experiments *expt);

int main(int argc, char **argv){
	
  int size, rank, rc, i;
  pthread_attr_t attr;
  void *res;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  rc = pthread_attr_init(&attr);
  assert(rc==0);

  get_options(argc, argv, &pbcfg, &mctx, &expt);
  mctx.size = size;
  mctx.schedrank = size-1;
  assert(size==(mctx.machines+1));
  mctx.expt = &expt;

  getinput(&pbcfg);/* load X, Y matrices */
  makecm(&pbcfg);  /* make colum major X matrix */
  DGPRINT(ERR, "input loading done in rank %d\n", rank);

  MPI_Barrier(MPI_COMM_WORLD);
  if(rank == 0){
    printcfg(&pbcfg, &mctx, &expt);
  }

  pthread_barrier_init(&mctx.bar_all,NULL, mctx.thrdpm+1);
  pthread_barrier_init(&mctx.bar_workers,NULL, mctx.thrdpm);
  /* create thread context for workers, and scheduler clients/server */
  create_threadcxt(&mctx, &pbcfg, rank);
  MPI_Barrier(MPI_COMM_WORLD); /* essential barrier don't remove */

  /* if client machine, create workers and scheduler-client */
  if(rank < mctx.machines){
    for(i=0; i<mctx.thrdpm; i++){
      rc = pthread_create(&thctx[i]->pthid, &attr, &worker, thctx[i]);
      assert(rc==0);
    }
    rc = pthread_create(&(schedc->pthid), &attr, &sched_client, schedc);
    assert(rc==0);
  }else{
    rc = pthread_create(&(scheds->pthid), &attr, &sched_server, scheds);
    assert(rc==0);
  }

  /* wait for completion of threads */
  if(rank < mctx.machines){ /* client machines */
    for(i=0; i<mctx.thrdpm; i++){
      rc = pthread_join(thctx[i]->pthid, &res);
      assert(rc==0);
    }
    rc = pthread_join(schedc->pthid, &res);
    assert(rc==0);
  }else{                   /* server machine */
    rc = pthread_join(scheds->pthid, &res);
    assert(rc==0);
  }

  MPI_Barrier(MPI_COMM_WORLD);
  DGPRINT(ERR, " rank %d leaving.\n", rank);
  MPI_Finalize();
  return 0;
}

static void create_threadcxt(struct context *ctx, struct problem *cfg, int rank){
  /* create thread context for lasso worker */
  thctx = (struct thrctx **)calloc(mctx.thrdpm, sizeof(struct thrctx *));
  for(int i=0; i<mctx.thrdpm; i++){
    thctx[i] = (struct thrctx *)calloc(1, sizeof(struct thrctx));
    assert(thctx[i] != NULL);
    thctx[i]->lid = i;
    thctx[i]->gid = rank*(mctx.thrdpm) + i;
    thctx[i]->rank = rank;
    thctx[i]->mctx = ctx;
    thctx[i]->pbcfg = cfg;
  }
  /* create thread context for scheduler server,client */
  scheds = (struct schedctx *)calloc(1, sizeof(struct schedctx));
  schedc = (struct schedctx *)calloc(1, sizeof(struct schedctx));
  scheds->rank = rank;
  scheds->mctx = ctx;
  scheds->pbcfg = cfg;

  schedc->rank = rank;
  schedc->mctx = ctx;
  schedc->pbcfg = cfg;

}

void printcfg(struct problem *cfg, struct context *mctx, struct experiments *expt){
  DGPRINT(ERR, "---------------------------- Problem Set ----------------------------\n");
  DGPRINT(ERR, "file-x: %s \t file-y: %s\n", cfg->xfile, cfg->yfile);
  DGPRINT(ERR, "samples: %07d \t features: %07d\n", cfg->samples, cfg->features); 
  DGPRINT(ERR, "machine: %07d \t thrd-machine: %d \t thrds: %d\n", 
	  mctx->machines, mctx->thrdpm, mctx->thrds);
  DGPRINT(ERR, "MPI_size: %d (total machine including scheduler) \nschedrank: %d\n", 
	  mctx->size, mctx->schedrank);
  DGPRINT(ERR, "max-iteration: %d \t timielimit: %d sec\n", 
	  expt->maxiter, expt->timelimit);
  DGPRINT(ERR, "--------------------------------------------------------------------\n");
}
