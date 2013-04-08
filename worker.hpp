
/********************************************************
  @author: Jin Kyu Kim (jinkyuk@cs.cmu.edu)
  Parallel Lasso problem solver based coordinate descent.

********************************************************/

#if !defined(WORKER_HPP_)
#define WORKER_HPP_
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


void *worker(void *arg);

#endif 
