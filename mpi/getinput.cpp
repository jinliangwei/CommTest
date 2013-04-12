/********************************************************
  @author: Jin Kyu Kim (jinkyuk@cs.cmu.edu)

********************************************************/
//#define _GNU_SOURCE 
#include "getinput.hpp"
using namespace std;

static void featurecount(char *fn, char *tmpbuf, int *samples, int *features){
  FILE *fd;
  char *ptr;
  int fcnt=0, scnt=0;

  fd = fopen(fn, "r");
  if(fd == NULL){
    DGPRINT(ERR, "fatal: feature count error \n");
    exit(0);
  }
  if(fgets(tmpbuf, LINEMAX, fd)){
    scnt++;
  }
  ptr = strtok(tmpbuf, DELIM2);
  while(ptr != NULL){
    fcnt++;
    ptr = strtok(NULL, DELIM2);
  }
  while(fgets(tmpbuf, LINEMAX, fd)){
    scnt++;
  }
  *samples = scnt;
  *features= fcnt; 
  DGPRINT(ERR, " %d samples found\n", *samples);
  DGPRINT(ERR, " %d features found\n", *features);
}

/* Standardize Matrix X by column */
static void standardizeInput(double **MX, double *MY, int n, int p){
  int i, j, row;
  double avg, dev, stdev, tmp;
  DGPRINT(INF, "Standardize X/Y\n");
  /* standardize n-by-p X  column by colum */
  for(i=0; i<p; i++){
    avg = 0; 
    dev = 0;
    stdev = 0;
    for(j=0; j<n; j++){
      avg += MX[j][i];
    }
    avg = avg/n;
    dev = 0;
    for(j=0; j<n; j++){
      dev += ((MX[j][i]-avg)*(MX[j][i]-avg))/n;
    }    
    stdev = sqrt(dev);
    /* TODO: CHECK HOW TO DO STANDARDIZE WHEN STDEV == 0) */
    if(stdev == 0.0){
      for(row=0; row<n; row++){
	if(MX[row][i] != 0.0){
	  DGPRINT(ERR, "fatal: stdev=0, MXelement nonzero, make [-nan]\n");
	  exit(0);
	}
      }
    }else{
      for(j=0; j<n; j++){
	tmp = ((MX[j][i]-avg)/stdev);      
	MX[j][i] = tmp/sqrt(n);      
      }
    }
  }
  /* normalize Y */
  avg = 0;
  dev = 0;
  stdev = 0;
  for(i=0; i<n; i++){
    avg += MY[i];
  }
  avg = avg/n;
  for(i=0; i<n; i++){
    dev += ((MY[i]-avg)*(MY[i]-avg))/n;
  }
  stdev = sqrt(dev);
  for(i=0; i<n; i++){
    tmp = ((MY[i]-avg)/stdev);
    MY[i] = tmp/sqrt(n);
  }
#if 0 
  /* print out sum of each columns and sum of each colums squared */
  /* SUM 1 should be zero, SUM 2 should be 1 after normalization
   * (Standardization)
   */
  double sum1, sum2
  for(i=0; i<p; i++){
    sum1 = 0; 
    sum2 = 0;
    for(j=0; j<n; j++){
      sum2 += (MX[j][i])*(MX[j][i]);
      sum1 += (MX[j][i]); 
    }
    DGPRINT(OUT, "col[%d] sum %lf == 0.0 / square_sum %lf == 1.0 \n", i, sum1, sum2);
  }
  sum1 = 0;
  sum2 = 0;
  for(i=0; i<n; i++){
    DGPRINT(OUT, "\n Y[%d] = [%lf] ",i, MY[i]);
    sum1 += (MY[i]);
    sum2 += (MY[i]*MY[i]);
  }
  DGPRINT(OUT, "\n Y sum %lf == 0.0 square_sum %lf == 1.0\n", sum1, sum2);
#endif 
}

void getinput(struct problem *pcfg){  
  char *ptr;
  int i, j;
  FILE *fdx, *fdy;
  double **tmpx, *tmpy;
  char *linebuf;

  fdx = fopen(pcfg->xfile, "r");
  fdy = fopen(pcfg->yfile, "r");
  
  if(!fdx || !fdy){
    DGPRINT(ERR, "fatal: %s %s files open failed \n", 
	    pcfg->xfile, pcfg->yfile);
    exit(0);
  }
  linebuf = (char *)calloc(LINEMAX, sizeof(char));
  featurecount(pcfg->xfile, linebuf, &pcfg->samples, &pcfg->features);

  /* allocate X matrix and read X file */
  tmpx = (double **)calloc(pcfg->samples, sizeof(double *));
  for(i=0; i<pcfg->samples; i++){
    tmpx[i] = (double *)calloc(pcfg->features, sizeof(double));
    if(!tmpx[i]){
      DGPRINT(ERR, "fatal: x mem alloc failed \n");
      exit(0);
    }
  }  
  i = 0;
  while(fgets(linebuf, LINEMAX, fdx) != NULL){
    j = 0;
    if(i >= pcfg->samples){
      DGPRINT(ERR, "fatal: input x has more samples \n");
      exit(0);
    }
    ptr = strtok(linebuf, DELIM2);
    while(ptr != NULL){
      if(j >= pcfg->features){
	DGPRINT(ERR, "fatal: input x has more features \n");
	exit(0);
      }
      sscanf(ptr, "%lf", &tmpx[i][j]);
      ptr = strtok(NULL, DELIM2);
      j++;
    }
    i++;
  }
  pcfg->X = tmpx;
  DGPRINT(OUT, "X loading done\n");

  /* alloc Y vector and read Y file */
  tmpy = (double *)calloc(pcfg->samples, sizeof(double));
  if(!tmpy){
    DGPRINT(ERR, "fatal: y mem alloc failed \n");
    exit(0);    
  }
  for(i=0; i<pcfg->samples;i++){
    if(fgets(linebuf, LINEMAX, fdy) == NULL){
      printf("Input Matrix Y is not open [%s]\n", linebuf);
      exit(0);
    }
    ptr = strtok(linebuf, DELIM2);
    sscanf(ptr, "%lf", &tmpy[i]);
  }
  pcfg->Y = tmpy;
  DGPRINT(OUT, "Y has [%d] samples loading done\n", i);

  /* standardize X and Y */
  standardizeInput(pcfg->X, pcfg->Y, pcfg->samples, pcfg->features);
  DGPRINT(ERR, "end stadndization\n");
  return;
}

void makecm(struct problem *cfg){

  int row, col, i;
  //  double sum, sqsum;

  cfg->cmX = (double **)calloc(cfg->features, sizeof(double *));
  for(i=0; i < cfg->features; i++){
    cfg->cmX[i] = (double *)calloc(cfg->samples, sizeof(double));
    if(cfg->cmX[i] == NULL){
      DGPRINT(ERR, "fatal: mem alloc failed \n");
      exit(0);
    }
  }

  if(cfg->cmX == NULL){
    DGPRINT(ERR, "fatal: mem alloc failed \n");
    exit(0);
  }
  for(row=0; row<cfg->samples; row++){
    for(col=0; col<cfg->features; col++){
      cfg->cmX[col][row] = cfg->X[row][col];
    }
  }

#if 0
  double sum, sqsum;
  for(row=0; row<cfg->features; row++){
    sum = 0;
    sqsum = 0;
    for(col=0; col<cfg->samples; col++){
      sum += cfg->cmX[row][col];
      sqsum += (cfg->cmX[row][col]*cfg->cmX[row][col]);
    }
    DGPRINT(ERR, "cmX[%d]: sum %lf sqsum %lf\n", row, sum, sqsum);
  }
#endif 
}
