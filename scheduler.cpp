#include <boost/crc.hpp>
#include <assert.h>
#include "scheduler.hpp"
#include "getinput.hpp"
#include "dynamic.hpp"
#include <queue>

using namespace std;
 
#define mpierr_handler(error_code, myrank)	\
  if(error_code != MPI_SUCCESS){					\
    char error_string[256];						\
    int length_of_error_string, error_class;				\
    MPI_Error_class(error_code, &error_class);				\
    MPI_Error_string(error_class, error_string, &length_of_error_string); \
    fprintf(stderr, "%3d: %s\n", myrank, error_string);			\
    MPI_Error_string(error_code, error_string, &length_of_error_string); \
    fprintf(stderr, "%3d: %s\n", myrank, error_string);			\
  }

#define checkResults(string, val) {		 \
    if (val) {						\
      printf("Failed with %d at %s", val, string);	\
      exit(1);						\
    }							\
  }


static void check_confgsanity(struct problem *, struct context *);
static void sendheartbeat(struct schedctx *ctx);
static void encappkt(packet *sdpkt, msg *sdmsg);
static uint32_t generate_crc32(char *buf, int len);
static int msg_checkcrc(packet *pkt);
static void initcomm(void);
static void waitforall(int clients, int myrank);
static void server_sendtestmsg(struct schedctx *ctx, int destrank);
msg *client_receivemsg(struct schedctx *ctx);
void client_sendtestmsg(struct schedctx *ctx);

static packet *client_async_receivemsg(struct schedctx *ctx, MPI_Request *request);
static int check_complete_async(MPI_Request *request);
static void client_sendtestmsg_wt_buffer(struct schedctx *ctx, packet *sdpkt);
static packet *server_async_receivemsg(struct schedctx *ctx, MPI_Request *request);

pthread_mutex_t mutx_s2w = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutx_w2s = PTHREAD_MUTEX_INITIALIZER;
queue<msg*>s2w;
queue<msg*>w2s;


static uint64_t timenow(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((uint64_t)(tv.tv_sec) * 1000000 + tv.tv_usec);
}

msg *worker_get_task(void){

  int rc, size;
  msg *ret=NULL;

  rc = pthread_mutex_lock(&mutx_s2w);
  checkResults("pthread_mutex_lock()\n", rc);
  if(!s2w.empty()){
    ret = s2w.front();
    s2w.pop();
    size = s2w.size();
    DGPRINT(DBG, "    worker_pop_task size %d\n", size);

  }
  rc = pthread_mutex_unlock(&mutx_s2w);
  checkResults("pthread_mutex_unlock()\n", rc);
  return ret;
}


void worker_putback_task(msg *element){
  int rc, size;

  rc = pthread_mutex_lock(&mutx_w2s);
  checkResults("pthread_mutex_lock()\n", rc);  
  w2s.push(element);
  size = w2s.size();
  DGPRINT(DBG, "    worker_push_completetask size %d\n", size);
  rc = pthread_mutex_unlock(&mutx_w2s);

  checkResults("pthread_mutex_unlock()\n", rc);
}


msg *client_putback_task(void){

  int rc, size;
  msg *ret=NULL;

  rc = pthread_mutex_lock(&mutx_w2s);
  checkResults("pthread_mutex_lock()\n", rc);
  if(!w2s.empty()){
    ret = w2s.front();
    w2s.pop();
    size = w2s.size();
    DGPRINT(DBG, "    client_pop_task size %d\n", size);

  }
  rc = pthread_mutex_unlock(&mutx_w2s);
  checkResults("pthread_mutex_unlock()\n", rc);
  return ret;
}



void client_put_task(msg *element){
  int rc, size;

  rc = pthread_mutex_lock(&mutx_s2w);
  checkResults("pthread_mutex_lock()\n", rc);  
  s2w.push(element);
  size = s2w.size();
  DGPRINT(DBG, "  client_push_newtask size %d\n", size);
  rc = pthread_mutex_unlock(&mutx_s2w);

  checkResults("pthread_mutex_unlock()\n", rc);
}

/* run scheduler job 
   TODO: add helper threads for the server thread 
*/
void *sched_server(void *arg){

  MPI_Request request;
  int i, j, complete;
  packet *donepacket;
  msg *rcmsg;
  uint64_t stime, etime, elapsems;

  struct schedctx *ctx = (struct schedctx *)arg;
  DGPRINT(OUT, "sched-server: rank %d init \n", ctx->rank);
  check_confgsanity(ctx->pbcfg, ctx->mctx);

  initcomm();
  waitforall(ctx->mctx->machines, ctx->rank); /* wait for heartbeat from all lasso worker machines */

  stime = timenow();
  while(1){
    for(i=0; i<ctx->mctx->machines; i++){
      for(j=0; j<ctx->mctx->thrdpm; j++){
	server_sendtestmsg(ctx, i);
      }
    }

    for(i=0; i<(ctx->mctx->machines*ctx->mctx->thrdpm); i++){
      donepacket = server_async_receivemsg(ctx, &request);
      while((complete = check_complete_async(&request)) == 0) {     			 
	/* prepare next task */

	/* do computation job */
      }
      assert(msg_checkcrc(donepacket) == CRC_OK);
      rcmsg = (msg *)donepacket->bin; 
      DGPRINT(DBG, "sched_server recv donemsg from rank %d status[%X] seq[%d]\n", 
	      rcmsg->src, rcmsg->status, rcmsg->seq);
      ctx->inmsg++;

      if((ctx->inmsg % 1000) == 0){
	etime = timenow();
	elapsems = (etime-stime)/1000;
	DGPRINT(ERR, "sched_server recv ack from r%d stat[%X] seq[%d] etime%ldms  %lf msg per ms\n", 
		rcmsg->src, rcmsg->status, rcmsg->seq, elapsems, (ctx->inmsg*1.0)/elapsems);
      }
    }
    assert(ctx->inmsg == ctx->outmsg);
    /* check computation job is enough
     if not, do more job and dispatch them */
  }

  return NULL;
}

/* this is helper thread of lass workers . 
   One machine has one communication thread 
*/
void *sched_client(void *arg){

  struct schedctx *ctx = (struct schedctx *)arg;
  msg *newtask;
  msg *donetask;
  packet *newpacket;
  packet *donepacket;
  MPI_Request request;
  int complete;

  DGPRINT(OUT, "sched-client: rank %d boot\n", ctx->rank); 

  initcomm();  
  sendheartbeat(ctx); /* register myself to the scheduler */
  pthread_barrier_wait(&(ctx->mctx->bar_all));

  while(1){
    newpacket = client_async_receivemsg(ctx, &request);
    while((complete = check_complete_async(&request)) == 0) {     			 
      if((donetask = client_putback_task()) != NULL){
	donepacket = (packet *)donetask;
	client_sendtestmsg_wt_buffer(ctx, donepacket);
	free(donepacket);
      }
    };

    assert(msg_checkcrc(newpacket) == CRC_OK);
    newtask = (msg *)newpacket->bin;
    client_put_task(newtask);   
    /* dispatch job to workers */
  }
  return NULL;
}


/* HELPER FUNCTIONS                                                */ 
/*******************************************************************/
/* this is sanity check of payload types in scheduler.hpp          */
/* CAVEAT: On adding new type of payload, please add checking code */
/*******************************************************************/
static void check_confgsanity(struct problem *pbcfg, struct context *ctx){
  unsigned int maxpayloadsize = MSGSIZE-MSGMETASIZE;

  if(sizeof(msg) > MSGSIZE){
    DGPRINT(ERR, "fatal: msg size is too large\n");
    exit(0);
  }  
  if(sizeof(betalist)>maxpayloadsize){
    DGPRINT(ERR, "fatal: betalist payload size is too large \n");
    exit(0);
  }
  if(sizeof(varlist)>maxpayloadsize){
    DGPRINT(ERR, "fatal: varlist payload size is too large\n");
    exit(0);
  }
  if(sizeof(heartbeat)>maxpayloadsize){
    DGPRINT(ERR, "fatal: heartbeat payload size is too large\n");
    exit(0);
  }
  if(pbcfg->samples > MAX_SAMPLE){
    DGPRINT(ERR, "fatal: the number of samples (residual entries) is too large. \n");
    exit(0);
  }
  DGPRINT(ERR, "sched_server: pass configuration sanity\n");
}

/* this is called by client/server */
void initcomm(void){
  char *tmp;
  int rc;
  tmp = (char *)calloc(1, MPIBUFSIZE);
  MPI_Errhandler_set(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
  rc = MPI_Buffer_attach(tmp, MPIBUFSIZE);
  mpierr_handler(rc, 0); 
}

static void sendheartbeat(struct schedctx *ctx){
  heartbeat *ht;
  msg *sdmsg;
  //uint32_t crc32bit, *crcpos;
  packet *sdpkt;

  sdpkt = (packet *)calloc(1, sizeof(packet));  /* to avoid memcpy */
  sdmsg = (msg *)&sdpkt->bin;
  assert(sdmsg != NULL);

  sdmsg->src = ctx->rank;
  sdmsg->dst = ctx->mctx->schedrank;
  ht = (heartbeat *)sdmsg->payload;
  ht->machinerank = ctx->rank;

  encappkt(sdpkt, sdmsg);
  DGPRINT(INF, "rank %d send ht to rank %d \n", ctx->rank, ctx->mctx->schedrank);
  MPI_Send((void *)sdpkt, MSGSIZE, MPI_BYTE, ctx->mctx->schedrank, COMMTAG, MPI_COMM_WORLD);
  free(sdpkt);
}


static uint32_t generate_crc32(char *buf, int len){ 
  boost::crc_32_type result;
  result.process_bytes(buf, len);
  DGPRINT(INF,"CRC: %X\n", result.checksum());
  return result.checksum();
}

static int msg_checkcrc(packet *pkt){
  uint32_t generated, *expected;
  int retval = CRC_ERR;

  expected = (uint32_t *)&pkt->bin[CRCPOS];
  generated = generate_crc32((char *)pkt->bin, CRCMSGSIZE);
  if(*expected == generated){
    retval = CRC_OK;
    DGPRINT(INF, "CRC match %X\n", *expected);
  }else{
    DGPRINT(ERR, "sched_server: fatal crc error\n");
  }
  
  return retval;
}

static void encappkt(packet *sdpkt, msg *sdmsg){
  uint32_t crc32bit, *crcpos;

  crc32bit = generate_crc32((char *)sdmsg, CRCMSGSIZE);
  DGPRINT(INF, "start %p   crc: %X \n", sdmsg, crc32bit);
  crcpos = (uint32_t *)&sdpkt->bin[CRCPOS];
  *crcpos = crc32bit;
}

static void waitforall(int clients, int myrank){
  MPI_Status mstatus;
  int livecnt = 0, pktlength=0;
  msg *rcmsg;
  packet *pkt;

  printf("WAIT FOR CLIENTS : %d\n", clients);

  pkt = (packet *)calloc(1, sizeof(packet));
  assert(pkt != NULL);
  rcmsg = (msg *)pkt->bin;

  while(livecnt < clients){
    MPI_Recv(pkt, MSGSIZE, MPI_BYTE, MPI_ANY_SOURCE, COMMTAG, MPI_COMM_WORLD, &mstatus);
    mpierr_handler(mstatus.MPI_ERROR, myrank);
    MPI_Get_count(&mstatus, MPI_BYTE, &pktlength);
    livecnt++;
    assert(msg_checkcrc(pkt) == CRC_OK);
    DGPRINT(ERR, "sched_server: rank %d (status src %d)is alive alive_vcnt %d mpi-info pktlen %d\n", 
	    rcmsg->src, mstatus.MPI_SOURCE, livecnt, pktlength);
    if(pktlength != MSGSIZE){
      DGPRINT(ERR, "@@@ warning: mpi receive pkt size is not equal to MSGSIZE \n");
    }
  }
  free(pkt);
}


msg *client_receivemsg(struct schedctx *ctx){
  MPI_Status mstatus;
  int pktlength=0;
  msg *rcmsg;
  packet *pkt;

  pkt = (packet *)calloc(1, sizeof(packet));
  assert(pkt != NULL);

  rcmsg = (msg *)pkt->bin; /* to avoid copy overhead */
  MPI_Recv(pkt, MSGSIZE, MPI_BYTE, ctx->mctx->schedrank, COMMTAG, MPI_COMM_WORLD, &mstatus);
  mpierr_handler(mstatus.MPI_ERROR, ctx->rank);
  MPI_Get_count(&mstatus, MPI_BYTE, &pktlength);

  DGPRINT(INF, "\tclient(%d) receive msg  mpi-info pktlen %d\n", ctx->rank,  pktlength);
  //crc32bit = generate_crc32((char *)pkt, CRCMSGSIZE);
  assert(msg_checkcrc(pkt) == CRC_OK);
  if(pktlength != MSGSIZE){
    DGPRINT(ERR, "@@@ warning: mpi receive pkt size is not equal to MSGSIZE \n");
  }
  
  return rcmsg;
  /*free(pkt);*/
}

packet *server_async_receivemsg(struct schedctx *ctx, MPI_Request *request){
  packet *pkt = (packet *)calloc(1, sizeof(packet));
  assert(pkt != NULL);

  MPI_Irecv(pkt, MSGSIZE, MPI_BYTE, MPI_ANY_SOURCE, COMMTAG, MPI_COMM_WORLD, request);
  return pkt; /* half done buffer don't touch until compelete is verified */
}


packet *client_async_receivemsg(struct schedctx *ctx, MPI_Request *request){

  packet *pkt = (packet *)calloc(1, sizeof(packet));
  assert(pkt != NULL);

  MPI_Irecv(pkt, MSGSIZE, MPI_BYTE, ctx->mctx->schedrank, COMMTAG, MPI_COMM_WORLD, request);
  return pkt; /* half done buffer don't touch until compelete is verified */
}
/* if asychronous recv is done, return non-zero 
    otherwise, it returns zero */
int check_complete_async(MPI_Request *request){     			 
  MPI_Status rstatus;
  int rflag;
  MPI_Test(request, &rflag, &rstatus);      
  return rflag;
}

void client_sendtestmsg(struct schedctx *ctx){
  heartbeat *ht;
  msg *sdmsg;
  packet *sdpkt;

  sdpkt = (packet *)calloc(1, sizeof(packet));  /* to avoid memcpy */
  sdmsg = (msg *)&sdpkt->bin;
  assert(sdmsg != NULL);

  sdmsg->src = ctx->rank;
  sdmsg->dst = ctx->mctx->schedrank;
  ht = (heartbeat *)sdmsg->payload;
  ht->machinerank = ctx->rank;

  encappkt(sdpkt, sdmsg);
  MPI_Bsend((void *)sdpkt, MSGSIZE, MPI_BYTE, ctx->mctx->schedrank, COMMTAG, MPI_COMM_WORLD);
  DGPRINT(DBG, "\tclient(%d)  send testmsg to sched-server\n", 
	  ctx->rank);

  free(sdpkt);
}

void client_sendtestmsg_wt_buffer(struct schedctx *ctx, packet *sdpkt){
  heartbeat *ht;
  msg *sdmsg;

  sdmsg = (msg *)&sdpkt->bin;
  assert(sdmsg != NULL);

  sdmsg->src = ctx->rank;
  sdmsg->dst = ctx->mctx->schedrank;
  ht = (heartbeat *)sdmsg->payload;
  ht->machinerank = ctx->rank;

  encappkt(sdpkt, sdmsg);
  MPI_Bsend((void *)sdpkt, MSGSIZE, MPI_BYTE, ctx->mctx->schedrank, COMMTAG, MPI_COMM_WORLD);
  DGPRINT(DBG, "\tclient(%d)  send testmsg to sched-server\n", 
	  ctx->rank);
}

static void server_sendtestmsg(struct schedctx *ctx, int destrank){
  heartbeat *ht;
  msg *sdmsg;
  packet *sdpkt;

  sdpkt = (packet *)calloc(1, sizeof(packet));  /* to avoid memcpy */
  sdmsg = (msg *)&sdpkt->bin;
  assert(sdmsg != NULL);

  sdmsg->seq = ctx->outmsg++;
  sdmsg->status = MSGSTATUS_UNDONE;
  sdmsg->src = ctx->rank;
  sdmsg->dst = destrank;
  ht = (heartbeat *)sdmsg->payload;
  ht->machinerank = ctx->rank;

  encappkt(sdpkt, sdmsg);
  DGPRINT(DBG, "server rank %d send testmsg to rank %d \n", 
	  ctx->rank, destrank);
  MPI_Bsend((void *)sdpkt, MSGSIZE, MPI_BYTE, destrank, COMMTAG, MPI_COMM_WORLD);
  free(sdpkt);
}
