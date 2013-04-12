
#include "../src/util.hpp"
#include "../src/threadpool.hpp"
#include <string>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
using namespace commtest;

int loglevel = 0;

struct thrinfo {
  int thrid;
  taskq* mq;
};

void *get_print_task(void * tinfo){
  while(1){
    task_t *m = ((thrinfo *) tinfo)->mq->poptask();
    printf("%s\n", m->data);
    m->cbk(m);
  }
  return NULL;
}

void *taskcbk(void *task){
  free((task_t *) task);

  return NULL;
}

/*
 * A simple TCP echo server
 */

int main(int argc, char *argv[]){
  const int thrn = 8;
  pthread_t thr[thrn];
  thrinfo tinfo[thrn];
  taskq mq;
  int task_num = 100;
  int i;
  for(i = 0; i < thrn; i++){
    tinfo[i].thrid = i;
    tinfo[i].mq = &mq;
    pthread_create(thr + i, NULL, get_print_task, tinfo + i);
  }

  for(i = 0; i < task_num; i++){
    task_t *m = (task_t *) malloc(sizeof(task_t));
    m->data = (char *) malloc(30*sizeof(char));
    sprintf(m->data, "hello message num: %d", i);
    m->len = strlen(m->data);
    m->cbk = taskcbk;
    mq.pushtask(m);
  }
  while(1);
}
