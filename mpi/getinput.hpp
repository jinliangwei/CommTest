
/********************************************************
  @author: Jin Kyu Kim (jinkyuk@cs.cmu.edu)
  Parallel Lasso problem solver based coordinate descent.

********************************************************/

#if !defined(GETINPUT_H_)
#define GETINPUT_H_

/* #define _GNU_SOURCE */ 
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <unistd.h>
#include <sched.h>
#include <errno.h>
#include <stdint.h>
#include <sys/time.h>

#define DELIM " \n,{}"
#define DELIM2 " \t	\n,   "
#define ABSDELTA (0.0000001)
#define ZEROLIMIT (0.0000000001)
#define FNMAX (512)
/* line buffer to read X, Y file 
   it can handle 2M of 20byte sized entry */ 
#define LINEMAX (2000000*20)
/* beta smaller than 10^-10 is considered as zero */

#define INF  1
#define DBG  2
#define ERR  3
#define OUT  4
#define MSGLEVEL ERR
#define DGPRINT(level, fmt, args...) if(level >= MSGLEVEL)fprintf(stderr, fmt, ##args)

/* problem configuration */
struct problem{
  double **X;
  double *Y;
  int features;
  int samples;
  int stnd;          /* flag for standardize */
  char *xfile;       /* design mtx file      */
  char *yfile;       /* observation vec file */
  char *xsum;        /* if standardized, it must be 0 */
  char *sqsum;       /* if standardized, it must be 1 */
  double **cmX;      /* for column access, make column major matrix */
};


struct experiments{
  int maxiter;       /* the number of operation dispatch */
  int timelimit;     /* seconds */
};

struct context{
  int machines;      /* the number of machines */
  int thrdpm;        /* the number of thread per machine */
  int thrds;         /* the number of total threads */
  double lamda;
  int size;          /* real number of machines including scheduler(s) */
  int schedrank;     /* scheduler's rank */
  pthread_barrier_t bar_all;
  pthread_barrier_t bar_workers;
  struct experiments *expt;
};


void getinput(struct problem *);
void makecm(struct problem *);
#endif 
