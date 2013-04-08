#include "dgraph.hpp"

using namespace std;

struct dgraphctx *dgctx;

void initdgraph(struct problem *cfg){

  int i, j;

  dgctx = (struct dgraphctx *)calloc(1, sizeof(struct dgraphctx));
  dgctx->features = cfg->features;

  /* allocate adjacency matrix for correlation graph */
  dgctx->adj = (double **)calloc(cfg->features, sizeof(double *));
  for(i=0; i<cfg->features; i++){
    dgctx->adj[i] = (double *)calloc(cfg->features, sizeof(double));
    assert(dgctx->adj[i]!= NULL);
    for(j=0; j<cfg->features; j++){
      dgctx->adj[i][j] = DGINVALIDENTRY;
    }
  }


  /* crreate a copy of adj matrix for recovery after diminising */
  dgctx->adjcopy = (double **)calloc(cfg->features, sizeof(double *));  
  for(i=0; i<cfg->features; i++){
    dgctx->adjcopy[i] = (double *)calloc(cfg->features, sizeof(double));
    assert(dgctx->adjcopy[i]!= NULL);
  }

  /* allocate working memory */
  dgctx->workmem = (double *)calloc(cfg->features, sizeof(double));
  if(dgctx->workmem == NULL){
    DGPRINT(ERR, "fatal: workmem alloc failed \n");
    exit(0);
  }

}

void getcorrinfo(struct problem *cfg){

  int i, j;
  double corr;

  DGPRINT(ERR, "making correlation graph take few seconds or mins\n");
  for(i=0; i<cfg->features-1; i++){
    if((i%1000) == 0)DGPRINT(OUT, ".");
    for(j=i+1; j< cfg->features; j++){
      corr = getcorr_pair(cfg->cmX[i], cfg->cmX[j], cfg->samples,0, 1, 0, 1, 1);
      dgctx->adj[i][j] = corr;
      DGPRINT(INF, "pair(%d, %d) corr: %lf\n", i, j, corr);
    }
  }

  /* fill out part under diagonal and diagonal for the convenience of searching */
  for(i=0; i<cfg->features-1; i++){
    for(j=i+1; j< cfg->features; j++){
      dgctx->adj[j][i] = dgctx->adj[i][j];
    }
    dgctx->adj[i][i] = DGINVALIDENTRY;
  }
  DGPRINT(ERR, "\ndgraph is ready\n");

  for(i=0; i<cfg->features; i++){
    for(j=0; j<cfg->features; j++){
      dgctx->adjcopy[i][j] = dgctx->adj[i][j];
    }
  }
  DGPRINT(ERR, "copy of graph is ready\n");
}

void recoverdgraph(struct problem *cfg){
  int i, j;
  for(i=0; i<cfg->features; i++){
    for(j=0; j<cfg->features; j++){
      dgctx->adj[i][j] = dgctx->adjcopy[i][j];
    }
  }
}


struct dispatchset tmpset;
/* set: container to keep a set of variables to be dispatched */
/* n  : the size of the set. */
/* return : 0 on success to find n ind set 
           -1 on failure due to the depletion 
*/
int findset(struct dispatchset *set, int n, struct problem *cfg){
  int i,j,k, t;
  struct corrpair minp;
  int remain=n;
  double maxcorr;
  int depleteflag = 0;

  minp.i =0;
  minp.j =0;
  minp.corr = 0.0;

  if(MAXVARLIST < n){
    DGPRINT(ERR, "fatal: size of dispatch set is too large \n");
    exit(0);
  }

  for(i=0; i<MAXVARLIST; i++){
    set->vlist[i] = -1;
    tmpset.vlist[i] = -1;
  }
  set->num=0;
  tmpset.num=0;

  minp.corr = 1000;
  for(i=0; i<cfg->features-1; i++){
    for(j=i+1; j<cfg->features; j++){
      if(dgctx->adj[i][j] == DGINVALIDENTRY)continue; 
      if(fabs(dgctx->adj[i][j]) < minp.corr){
	minp.i = i;
	minp.j = j;
	minp.corr = fabs(dgctx->adj[i][j]);
      }
    }
  }

  if(minp.corr == 1000){
    DGPRINT(INF, "min corr is not found as graph is depleted completely \n");
    set->num =0;
    return -1;
  }

  set->vlist[0] = minp.i;
  set->vlist[1] = minp.j;
  diminishgraphcol(minp.i, cfg);
  diminishgraphcol(minp.j, cfg);

  set->num = 2;
  tmpset.num = 0;
  dgctx->adj[minp.i][minp.j] = DGINVALIDENTRY;

  DGPRINT(INF, "minp.i %d minp.j %d corr %lf\n", minp.i, minp.j, minp.corr);

  while(remain>0){
    minp.corr = 1000;
    DGPRINT(INF, "@@@ iteration \n");
    //    for(i=0; i<set->num; i++){
    for(i=0; i<1; i++){
      t = set->vlist[i];

      DGPRINT(INF, "for a pocket item  %d \n", t);
      for(j=0; j<cfg->features; j++){
	dgctx->workmem[j]=1000;
      }
      for(j=0; j<cfg->features; j++){
	if(dgctx->adj[t][j] == DGINVALIDENTRY)continue;
	//maxcorr = -1000;
	maxcorr = 0;
	for(k=0; k<set->num; k++){
	  /* printf("set->vlist[k][j] :%d \n", set->vlist[k]);  */
	  /*if(fabs(dgctx->adj[set->vlist[k]][j]) > maxcorr){
	    maxcorr = fabs(dgctx->adj[set->vlist[k]][j]);
	    }*/
	  maxcorr += fabs(dgctx->adj[set->vlist[k]][j]);
	}
	dgctx->workmem[j] = maxcorr;
      } /* for(j=0; j<cfg->features */

      minp.corr = 1000;
      for(j=0; j<cfg->features; j++){
	if(minp.corr > dgctx->workmem[j]){
	  minp.corr = dgctx->workmem[j];
	  minp.i = j;
	}
      }
      
      tmpset.vlist[tmpset.num] = minp.i;
      DGPRINT(INF, "chosen minp.i: %d corr %lf\n", minp.i, minp.corr);
      if(minp.corr == DGINVALIDENTRY){
	depleteflag = -1;
	tmpset.num--;
	break;
      }
      dgctx->adj[t][minp.i] = DGINVALIDENTRY;
      diminishgraphcol(minp.i, cfg);
      tmpset.num++;
      if(tmpset.num + set->num >= n){
	break;
      }
    } /* for i<set->num */

    for(i=0; i<tmpset.num; i++){
      set->vlist[set->num] = tmpset.vlist[i];
      set->num++;
    }
    tmpset.num = 0;
    for(i=0; i<MAXVARLIST; i++){
      tmpset.vlist[i] = -1;
    }

    for(i=0; i<set->num; i++){
      DGPRINT(INF, "set->vlist[%d]: %d\n", i, set->vlist[i]);
    }
    
    if(depleteflag == -1){
      DGPRINT(INF, "exit with deplete flag -1\n");
      break;
    }

    if(set->num >= n)break;
  } /* while */


  for(i=0; i<set->num; i++){
    diminishgraphcol(set->vlist[i], cfg);
    diminishgraphrow(set->vlist[i], cfg);
  }
  
  return depleteflag;
}


/* set a whole column to invalid since it's chosen. */
void diminishgraphcol(int col, struct problem *cfg){
  int row;
  for(row=0; row < cfg->features; row++){
    dgctx->adj[row][col] = DGINVALIDENTRY;
  }
}

void diminishgraphrow(int row, struct problem *cfg){
  int col;
  for(col=0; col < cfg->features; col++){
    dgctx->adj[row][col] = DGINVALIDENTRY;
  }
}


/* take a snapshot of correlation graph, 
   and stor it into a file with given xfile name + .corr */
void snapshotdbgraph(struct problem *cfg){
  
  int row, col;
  FILE *fp;
  char *tmpfn;

  tmpfn = (char *)calloc(strlen(cfg->xfile)+10, sizeof(char));
  sprintf(tmpfn, "%s.corr", cfg->xfile);

  DGPRINT(ERR, "storing correlation graph adj file %s\n", tmpfn);
  fp = (FILE *)fopen(tmpfn, "wt");
  assert(fp != NULL);

  for(row=0; row < cfg->features; row++){
    for(col=0; col <cfg->features; col++){
      fprintf(fp, " %2.20lf ", dgctx->adj[row][col]);
    }
    fprintf(fp, "\n");
  }
  fclose(fp);
}

/* read snapshot file of correlation graph. */
void loadsnapshot(struct problem *cfg){

  int row=0, col=0;
  FILE *fp;
  char *buf, *ptr;
  char *tmpfn;

  for(row=0; row < cfg->features; row++){
    for(col=0; col <cfg->features; col++){
      dgctx->adj[row][col] = DGINVALIDENTRY;
    }
  }

  tmpfn = (char *)calloc(strlen(cfg->xfile)+10, sizeof(char));
  sprintf(tmpfn, "%s.corr", cfg->xfile);
  DGPRINT(ERR, "loading correlation graph adj file %s\n", tmpfn);

  fp = (FILE *)fopen(tmpfn, "rt");
  assert(fp != NULL);

  /* 80MB buffer */
  buf = (char *)calloc(LINEMAX, sizeof(char));
  assert(buf != NULL);
  
  row =0;
  while(fgets(buf, LINEMAX, fp) != NULL){
    printf("row %d", row);
    col = 0;
    ptr = strtok(buf, DELIM);
    while(ptr != NULL){
      dgctx->adj[row][col] = atof(ptr);
      col++;
      ptr = strtok(NULL, DELIM);
    }
    assert(col==cfg->features);
    row++;
  }
  printf("\n");

  assert(row == cfg->features);
  DGPRINT(ERR, "loading corr %s file is done with %d by %d \n", tmpfn, row, col);
  fclose(fp);

  for(row=0; row < cfg->features; row++){
    for(col=0; col <cfg->features; col++){
      dgctx->adjcopy[row][col] = dgctx->adj[row][col];
    }
  }
  DGPRINT(ERR, "dgctx->adjcopy is filled out. \n");

#if 0 
  for(row=0; row < cfg->features; row++){
    for(col=0; col <cfg->features; col++){
      if(fabs(fabs(dgctx->adj[row][col]) - fabs(dgctx->adjcopy[row][col])) > (0.000000000000000001) ){
	DGPRINT(ERR, "at %d %d, mismatch %2.20lf %2.20lf delta %2.20lf threshold %2.20lf\n", 
		row, col, dgctx->adj[row][col], dgctx->adjcopy[row][col], 
		fabs(fabs(dgctx->adj[row][col]) - fabs(dgctx->adjcopy[row][col])), (0.000000000000000001));
	mcnt++;
      }
    }
  }
  DGPRINT(ERR, "loading done mismatch cnt %d\n", mcnt);
#endif 

}
