#include <pthread.h>
#include "suma.h"
#include <stdio.h>
typedef unsigned long long Set;
int cores=8;
// Defina aca las estructuras que necesite

typedef  struct{
int ini;
int fin;
Set res;
int len;
int *arr;
}Args;

// Defina aca la funcion que ejecutaran los threads
void * thread(void * p) {
    Args * args =(Args *)p;
    int ini =args->ini;
    int fin=args->fin;
    int len=args->len;
    int *a =args->arr;
        for (int k= ini; k<=fin; k++) {
            // k es el mapa de bits para el subconjunto { a[i] | bit ki de k es 1 }
            long long sum= 0;
            for (int i= 0; i<len; i++) {
                if ( k & ((Set)1<<i) ){ // si bit ki de k es 1
                    sum+= a[i];
                }
            }
            if (sum==0) { // éxito: el subconjunto suma 0
                args->res = k;
                return NULL; // y el mapa de bits para el subconjunto es k
                } 
        }
    args->res = 0;
    return NULL;
}

// Reprograme aca la funcion buscar
Set buscar(int a[], int n) { 
 pthread_t pid[cores];
 Set comb= (1<<(n-1)<<1)-1; 
 int len_interval = comb/8;
 Args args[cores];

 for(int i=0;i<cores;i++){
    args[i].ini=1+len_interval*i;
    args[i].fin=(len_interval*(i+1))-1;
    args[i].len= n;
    args[i].arr = a;
    pthread_create(&pid[i],NULL,thread,&args[i]);
 }
for (int k=0; k<cores;k++){
    pthread_join(pid[k],NULL);
    if(args[k].res !=0){
        return args[k].res;
    }
}
return 0;
}
