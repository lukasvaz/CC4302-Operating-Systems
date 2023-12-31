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
  //printf("llamada de %p a %p timeout %d status %d\n",(void * )this,(void * )th,timeout,this->status);
  
  Param *req=malloc(sizeof(Param));
  req->self_msg=msg;
  req->thread=th;
  req->response=0;

  this->ptr=req;
 
  Param  *th_ptr=th->ptr;
  Param  *this_ptr=this->ptr;

 //hize un refactoring de la T4, pasa los test anteriores!
 
 //si no es un thread respuesta -> veo estado
 while(!this_ptr->response){ 
 // si está en wait, veo si llamó a this
  if((th->status)==WAIT_EXCHANGE || (th->status)==WAIT_EXCHANGE_TIMEOUT ){

      nThread reqthread=th_ptr->thread;
      if(this==reqthread){
      //activo  thread respuesta
      
      //sin timeout
      if((th->status)==WAIT_EXCHANGE){
        void * res=th_ptr->self_msg;
        th_ptr->self_msg=msg;//intercambio de msg
        th_ptr->response=1;
        setReady(th);
        free(this->ptr);
        END_CRITICAL;
        return res;}
    
      //con timeout
      if((th->status)==WAIT_EXCHANGE_TIMEOUT){
        nth_cancelThread(th);
        void * res=th_ptr->self_msg;
        th_ptr->self_msg=msg;//intercambio de msg
        th_ptr->response=1;
        setReady(th);
        free(this->ptr);
        END_CRITICAL;
        return res;
      }
    }
  }
  //si th no está en algun wait suspendo this
    if (timeout>0){
      suspend(WAIT_EXCHANGE_TIMEOUT);
      nth_programTimer(timeout*1000000LL,NULL);
      schedule();
      if (!(this_ptr->response)){
      free(this->ptr);
      END_CRITICAL;
      return NULL;}
    }
    else{
      suspend(WAIT_EXCHANGE);
      schedule();
    }
  }
  //this es thread respuesta-> retorna directamente 
    if (this_ptr->response){
    
    void * res=this_ptr->self_msg;
    free(this->ptr);
    END_CRITICAL;
    return res;
  }
  free(this->ptr);
  END_CRITICAL;
  return NULL;
    
}
