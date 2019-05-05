#include "t_lib.h"

struct tcb
{
  int thread_id;
  int thread_priority;
  ucontext_t *thread_context;
  struct tcb *next;

};

typedef struct tcb tcb;
tcb *running;
tcb *ready;


void t_yield()
{
  tcb *temp;
  temp = running;
  temp->next = NULL;

  if(ready != NULL){
    running = ready;
    ready = ready->next;
    running->next=NULL;
    tcb *temp2;
    temp2 = ready;
    if(temp2 == NULL){
      ready = temp;
    }
    else{
      while(temp2->next != NULL){
        temp2 = temp2->next;
      }
      temp2->next = temp;
    }
  }

  swapcontext(temp->thread_context,running->thread_context);
}

void t_init()
{
  tcb *temp = (tcb*)(malloc(sizeof(tcb)));
  temp->thread_context = (ucontext_t *) malloc(sizeof(ucontext_t));
  getcontext(temp->thread_context);
  temp->next = NULL;
  temp->thread_id = 0;
  running = temp;
  ready = NULL;
}

int t_create(void (*fct)(int), int id, int pri)
{
  size_t sz = 0x10000;

  tcb *temp = (tcb*)(malloc(sizeof(tcb)));
  temp->thread_context = (ucontext_t *) malloc(sizeof(ucontext_t));
  getcontext(temp->thread_context);
  temp->thread_id = pri;
  temp->thread_id = id;
  temp->next = NULL;

/***
  uc->uc_stack.ss_sp = mmap(0, sz,
       PROT_READ | PROT_WRITE | PROT_EXEC,
       MAP_PRIVATE | MAP_ANON, -1, 0);
***/
   temp->thread_context->uc_stack.ss_sp = malloc(sz);  /* new statement */
   temp->thread_context->uc_stack.ss_size = sz;
   temp->thread_context->uc_stack.ss_flags = 0;
   temp->thread_context->uc_link = running->thread_context; 
  makecontext(temp->thread_context, (void (*)(void)) fct, 1, id);
  if(ready == NULL){
    ready = temp;
  }
  else{
    tcb* looper = ready;
    while(looper->next != NULL){
      looper = looper->next;
    }
    looper->next = temp;
  }
}

void t_shutdown(void){
  if(ready != NULL){
    tcb *temp ;
    while(ready != NULL){
      temp = ready->next;
      free(temp->thread_context->uc_stack.ss_sp);
      free (ready->thread_context);
      free(ready);
      ready = temp;
    }
  }
  free(running->thread_context);
  free(running);
}

void t_terminate(){
  tcb* temp;
  temp = running;
  running = ready;
  if(ready != NULL){
    ready = ready->next;
  }
  free(temp->thread_context->uc_stack.ss_sp);
  free (temp->thread_context);
  free(temp);
  setcontext(running->thread_context);
}