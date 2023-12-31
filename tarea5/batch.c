#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/time.h>
  
#include "pss.h"
#include "batch.h"

struct job {
int ready;
JobFun f;
void *input;
pthread_cond_t *pcond;
void *res; 
};

// ... defina aca sus variables globales ...
 
pthread_t threads[10000];
Queue *q;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  non_empty= PTHREAD_COND_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;

pthread_t *arr;
int num_threads = 0;
int done_jobs=0;
int requested_jobs=0;
int stop;

// ... defina las funciones auxiliares que necesite ...
void *batch(void * n) {
 while(1){
    pthread_mutex_lock(&m);
    
    while(emptyQueue(q) && (stop==0)){
      pthread_cond_wait(&non_empty,&m);
    }
    
    if(stop==1 ){
      // "thread killed"
      pthread_mutex_unlock(&m);
      return NULL;
    }
    Job  *j=get(q);
    pthread_mutex_unlock(&m);
    JobFun  function=j->f;
    void * respuesta=function(j->input);
    // "job calculated"
    pthread_mutex_lock(&m);
    j->res=respuesta;
    j->ready=1;
    pthread_cond_signal((j->pcond));
    pthread_mutex_unlock(&m);
  }
 return NULL;
}


void startBatch(int n) {
  stop=0;
  num_threads=0;
  requested_jobs=0;
  done_jobs=0;
  q=makeQueue();
  for (int i=0;i<n;i++){
    pthread_create(&threads[i],NULL,batch,NULL);
    num_threads++;
  }
}

void stopBatch() {
  pthread_mutex_lock(&m);
  //esta condicion funciona pq stop() se llama una 
  //vez terminados los submits (desde un mismo thread)
  while(requested_jobs!=done_jobs) {
    // " not empty"
    pthread_cond_wait(&empty,&m);
  }
  stop=1;
  pthread_mutex_unlock(&m);
  pthread_cond_broadcast(&non_empty);
  
  for (int i = 0; i < num_threads; i++) {
    pthread_join(threads[i], NULL);
  }
  destroyQueue(q);
  }

Job *submitJob(JobFun fun, void *input) {
  pthread_mutex_lock(&m);
  Job *pnewjob=malloc(sizeof(Job));
  pthread_cond_t c=PTHREAD_COND_INITIALIZER;
  pthread_cond_t *pc=malloc(sizeof(c));
  *pc=c;

  //creating new job
  pnewjob->pcond=pc;
  pnewjob->ready=0;
  pnewjob->f=fun;
  pnewjob->input=input;

  put(q,pnewjob);
  pthread_cond_signal(&non_empty);
  requested_jobs++;
  pthread_mutex_unlock(&m);
  return pnewjob;
}

void *waitJob(Job *job) {

  pthread_mutex_lock(&m);
  // printf("wait\n");
  while (!(job->ready)){
    //"job not ready wait for job cond";
    pthread_cond_wait(job->pcond,&m);
  }
  // "job ready %d\n"
  void* resultado=job->res;
  free(job->pcond);
  free(job);
  done_jobs++;

  if(requested_jobs==done_jobs){
    // "all jobs done, signal to stop";
    pthread_cond_signal(&empty);
  }
  pthread_mutex_unlock(&m);
  return resultado;
}

