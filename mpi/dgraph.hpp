#if !defined(DGRAPH_HPP_)
#define DGRAPH_HPP_

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "dynamic.hpp"
#include "getcorr.hpp"
#include "getinput.hpp"

#define DGINVALIDENTRY (1000)
#define MAXVARLIST (4096)

struct dgraphctx{
  double **adj;
  double **adjcopy;  /* copy for recovery */
  int features;
  int threshold;
  int minthreshold;
  int nzedge;
  int nznodes;
  double *workmem; /* size is features */
};

struct dispatchset{
  int vlist[MAXVARLIST];
  int num;
};

struct corrpair{
  int i;
  int j;
  double corr;
};

extern struct dgraphctx *dgctx;
/* allocate memory structure for correlation graph */
void initdgraph(struct problem *cfg);

/* make correlation graph for a given X matrix */
void getcorrinfo(struct problem *cfg);

/* recover diminised graph int original status after finishing one iteration */
void recoverdgraph(struct problem *cfg);

/* Try to find n variables with minimal correlation */
int findset(struct dispatchset *set, int n, struct problem *cfg);

/* invalidate a whole column when the column is selected */
void diminishgraphcol(int col, struct problem *cfg);

/* invalidate a whole row when the column is selected */
void diminishgraphrow(int col, struct problem *cfg);

/* take a snapshot of a graph and store it into a file */
void snapshotdbgraph(struct problem *cfg);

/* load a snapshot file into memory structure */
void loadsnapshot(struct problem *cfg);

#endif 
