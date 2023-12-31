#include "nthread-impl.h"

// Use los estados predefinidos WAIT_EXCHANGE y/o WAIT_EXCHANGE_TIMEOUT
// El descriptor de thread incluye el campo ptr para que Ud. lo use
// a su antojo.

//... defina aca las estructuras que necesite ...

typedef struct{
void *self_msg;//mensaje del propio thread
nThread thread;// thread al que se le pide mensaje
int response;
}Param;



void* nExchange(nThread th, void *msg, int timeout) {
  //... complete ...
  START_CRITICAL;
  nThread this=nSelf();
  
  Param *req=malloc(sizeof(Param));
  req->self_msg=msg;
  req->thread=th;
  req->response=0;

  this->ptr=req;

 //si thread no esta  en WAIT  no ha pedido exchange()
  if((th->status)!=WAIT_EXCHANGE){
    suspend(WAIT_EXCHANGE);
    schedule();
  }
  Param  *th_ptr=th->ptr;
  Param  *this_ptr=this->ptr;
 
  //si th no habia preguntado por this,espera
  nThread reqthread=th_ptr->thread;
  while(this!=reqthread && !(this_ptr->response)){
    suspend(WAIT_EXCHANGE);
    schedule();
  }
  //si pareja retorna(reponse =1),this bypasea  verificaciones y retorna directamente 
    if (this_ptr->response){
    END_CRITICAL;
    void * res=this_ptr->self_msg;
    free(this->ptr);
    return res;
  }

  //activo  thread respuesta
  void *  res=th_ptr->self_msg;
  th_ptr->self_msg=msg;//intercambio de msg
  th_ptr->response=1;
  setReady(th);
  END_CRITICAL;
  return res;  
}
