#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

#include "disk.h"

//... defina aca sus variables globales ...
int last_track;
int  requests[MAXTRACK];

pthread_mutex_t mutex;
pthread_cond_t cond;
//... defina aca otras funciones si las necesita ...

int findNext(int last,int* arr){
//obtiene  el turno de solicitud 
int val=0;
for (int i=last;i<MAXTRACK;i++){
  if(arr[i]>0){
    return i;
  } 
}

for (int i=0;i<last;i++){
  if(arr[i]>0){
    return i;
  } 
}
return val;
}
void requestDisk(int track) {
//req for mutex
  pthread_mutex_lock(&mutex);
  requests[track]++;
  while(track!=findNext(last_track,requests) ||requests[track]>1){
    pthread_cond_wait(&cond,&mutex);
  }
  last_track=findNext(last_track,requests);
  pthread_mutex_unlock(&mutex);

}

void releaseDisk(void) {
  pthread_mutex_lock(&mutex);
  requests[last_track]--;
  pthread_cond_broadcast(&cond);
  pthread_mutex_unlock(&mutex);
}

void diskInit(void) {
  //initialize array  in 0s
  for(int i=0;i<MAXTRACK;i++){
    requests[i]=0;
  }
  //initialize mutex and cond
  pthread_mutex_init(&mutex,NULL);
  pthread_cond_init(&cond,NULL);
  //last_track initialize 0
  last_track=0;
}

void diskClean(void) {
  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&cond);
}
