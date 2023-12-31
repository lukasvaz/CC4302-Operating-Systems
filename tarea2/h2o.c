#include <pthread.h>

#include "pss.h"
#include "spinlocks.h"
#include "h2o.h"

// ... defina aca variables globales ...
typedef struct{
  void* atom;
  int* spin_lock;
  H2O* h_2o;
  }Request;

 Queue* queue_H;
 Queue* queue_O;
 int mutex=OPEN;

void initH2O(void) {
  // ... complete ...
  queue_H =makeQueue();
  queue_O =makeQueue();
}

void endH2O(void) {
  // ... complete ...
  destroyQueue(queue_O);
  destroyQueue(queue_H);
}

H2O *combineOxy(Oxygen *o) {
  // ... complete ...
  spinLock(&mutex);
  if(queueLength(queue_H)>=2){
    Request* req_1=get(queue_H);
    Request* req_2=get(queue_H);
    H2O* h2_o=makeH2O(req_1->atom,req_2->atom,o);
    req_1->h_2o=h2_o;
    req_2->h_2o=h2_o;
    spinUnlock(req_1->spin_lock);
    spinUnlock(req_2->spin_lock);
    spinUnlock(&mutex);
    return h2_o;
  }
  else{
    int spin_lock=CLOSED;
    Request request={o,&spin_lock,NULL};
    put(queue_O,&request);
    spinUnlock(&mutex);
    spinLock(&spin_lock);
    spinLock(&mutex);
    H2O* res=(&request)->h_2o;
    spinUnlock(&mutex);
    return res;
  }
}

H2O *combineHydro(Hydrogen *h) {
  // ... complete ...
spinLock(&mutex);
if(queueLength(queue_H)>0 && queueLength(queue_O)>0){
    Request* req_1=get(queue_H);
    Request* req_2=get(queue_O);
    H2O* h2_o=makeH2O(req_1->atom,h,req_2->atom);
    req_1->h_2o=h2_o;
    req_2->h_2o=h2_o;
    spinUnlock(req_1->spin_lock);
    spinUnlock(req_2->spin_lock);
    spinUnlock(&mutex);
    return h2_o;
}
else{
    int spin_lock=CLOSED;
    Request request={h,&spin_lock,NULL};
    put(queue_H,&request);
    spinUnlock(&mutex);
    spinLock(&spin_lock);
    spinLock(&mutex);
    H2O* res=(&request)->h_2o;
    spinUnlock(&mutex);
    return res;
}
}
