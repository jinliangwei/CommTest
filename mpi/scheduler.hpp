

#if !defined(SCHEDULER_H_)
#define SCHEDULER_H_

#include <mpi.h>

#define MSGSIZE (8192)  /* total messaage size: packet size at app level */
#define MAX_PATCH (256) /* max number of entries in one dispatch */
#define MAX_SAMPLE (500) /* max number of residuals that is equal to the number of samples. */


#define CRCMSGSIZE (MSGSIZE-8)

#define MPIBUFSIZE (1024*1024*256)  /* Associate 256MB buffer to MPI run time */
#define COMMTAG (1521)              /* MPI requires it to be less than 32767*/ 

#define CRC_OK   (1)
#define CRC_ERR  (-1)
#define CRCPOS   (MSGSIZE-4)



/* TODO : SANITY check */

typedef struct{
  int var;
  double beta;
}varbeta;
/********************** payload type *************************************************/
#define CSHEARTBEAT   (0x10)  /* client to server : alive report                      */
#define SCTASKTYPE1   (0x20)  /* server to client : for first step - univariate reg   */
                              /* struct varlist is used                               */
#define SCTASKTYPE2   (0x30)  /* server to client : for all other steps - normal task */
                              /* struct varlist is used                               */
#define CSBETALIST    (0x40)  /* client to server : new beta values                   */
                              /* return new value of variables                        */

#define MSGSTATUS_UNDONE (0x50)
#define MSGSTATUS_DONE  (0x60)

typedef struct{
  int num;
  int vlist[MAX_PATCH];
  double rlist[MAX_SAMPLE];
}varlist;

/* beta list */
typedef struct{
  int num;
  varbeta paris[MAX_PATCH];
}betalist;

typedef struct{
  int machinerank;
}heartbeat;

/***************************************************************************/
#define MSGMETASIZE (48)
typedef struct{
  int src;
  int dst;
  int seq;   /* client does not change sequence number: when sending back,
		use the same sequence number */
  int type;  /* define CSHEARTBEAT .. CSBETALIST only */
  int status;
  char payload[MSGSIZE-MSGMETASIZE]; /* MSGMETASIZE : size of src, dst, seq, type + 8 byte for crc */
}msg;

typedef struct{
  char bin[MSGSIZE];
}packet;



void *sched_server(void *arg);
void *sched_client(void *arg);

msg *worker_get_task(void);
void worker_putback_task(msg *element);

void client_put_task(msg *);
msg *client_putback_task(void);



#endif
