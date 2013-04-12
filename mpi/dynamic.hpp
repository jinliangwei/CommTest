
/********************************************************
  @author: Jin Kyu Kim (jinkyuk@cs.cmu.edu)
  Parallel Lasso problem solver based coordinate descent.

********************************************************/

#if !defined(DYNAMIC_HPP_)
#define DYNAMIC_HPP_
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <sched.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <sys/time.h>
#include <string>
#include <iostream>
#include <pthread.h>
#include "scheduler.hpp"

#if !defined(SINGLEMODE)
#include <mpi.h>
#endif 

struct thrctx{
  pthread_t pthid;
  int lid;
  int gid;
  int rank;
  long start; /* ragne of a thread, column of first element in the pair. */
  long end;
  long load;
  struct context *mctx;
  struct problem *pbcfg;
};

struct schedctx{
  pthread_t pthid;
  int rank;
  int flag;
  int inmsg;
  int outmsg;
  struct context *mctx;
  struct problem *pbcfg;
};

#endif 
