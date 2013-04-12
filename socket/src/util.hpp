#ifndef __COMMTEST_UTIL_H__
#define __COMMTEST_UTIL_H__

#include <stdio.h>
#include <pthread.h>
#include <queue>
#include <sys/types.h>
#include <stdint.h>

#define DBG 0  // print when debugging
#define ERR 1  // print when error should be exposed
#define NOR 2  // always print

extern int loglevel; // user needs to define

#define LOG(LEVEL, OUT, FMT, ARGS...) if(LEVEL >= loglevel) fprintf(OUT, "[%s][%s][ln:%d]  "FMT, __FILE__, __FUNCTION__, __LINE__, ##ARGS)

namespace commtest {
  class semaphore{
  private:
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    uint32_t counter;
    
  public:
    // throw negative numbers if error
    semaphore(uint32_t _counter){
      if(_counter < 0) throw -1;
      pthread_mutex_init(&mutex, NULL);
      pthread_cond_init(&cond, NULL);
      counter = _counter;
    }

    ~semaphore(){
      pthread_mutex_destroy(&mutex);
      pthread_cond_destroy(&cond);
    }

    void signal(){
      pthread_mutex_lock(&mutex);
      counter++;
      pthread_mutex_unlock(&mutex);
      pthread_cond_signal(&cond);
    }

    void signal(uint32_t n){
      pthread_mutex_lock(&mutex);
      counter += n;
      pthread_mutex_unlock(&mutex);
      uint32_t i = 0;
      for(i = 0; i < n; i++){
	pthread_cond_signal(&cond);
      }
    }
    
    void wait(){
      pthread_mutex_lock(&mutex);
      while(counter == 0){
	pthread_cond_wait(&cond, &mutex);
      }
      counter--;
      pthread_mutex_unlock(&mutex);
    }
  };

  template<class T>
  class pcqueue{ //producer-consumer queue
  private:
    std::queue<T> q;
    semaphore sem;
    pthread_mutex_t mutex;
  public:
    pcqueue():
      sem(0){
      pthread_mutex_init(&mutex, NULL);
    }

    ~pcqueue(){
      pthread_mutex_destroy(&mutex);
    }

    void push(T t){
      pthread_mutex_lock(&mutex);
      q.push(t);
      pthread_mutex_unlock(&mutex);
      sem.signal();
    }

    void push(T t[], int n){
      pthread_mutex_lock(&mutex);
      int i;
      for(i = 0; i < n; i++){
	q.push(t[i]);
      }
      pthread_mutex_unlock(&mutex);
      sem.signal(n);
    }

    T pop(){
      sem.wait(); //semaphore lets no more threads in than queue size
      pthread_mutex_lock(&mutex);
      T t = q.front();
      q.pop();
      pthread_mutex_unlock(&mutex);
      return t;
    }
  };

  int readn(int fd, uint8_t *buff, size_t len);
  int writen(int fd, uint8_t *buff, size_t len);
}

#endif
