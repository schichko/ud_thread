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


typedef struct {
       int count;
       tcb *q;
} sem_t;

struct messageNode{
  char *message;
  int len;
  int sender;
  int receiver;
  struct  messageNode *next;  
};

typedef struct 
{
  struct messageNode *msg;
  sem_t *mbox_sem;
}mbox;


void t_yield()
{
  //printf("In Yeild\n");
  tcb *temp;
  temp = running;
  temp->next = NULL;
  //printf("Ready:%d\n",ready->thread_id);
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
  //printf("Running:%d\n",running->thread_id);
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


int sem_init(sem_t **sp, int sem_count)
{
  *sp = malloc(sizeof(sem_t));
  (*sp)->count = sem_count;
  (*sp)->q = NULL;
}

void sem_wait(sem_t *s)
{

 // printf("S Count In wait Start: %d\n",s->count);
  s->count = s->count -1;

/*
For semaphore operations, if sem_wait() decides that the calling
thread needs to block itself, it will
(1) remove the TCB of the "calling" thread from the RUNNING
    queue and insert it to the end of this semaphore's queue,
(2) promotes the first TCB of the READY queue into the RUNNING
queue, and
(3) calls swapcontext().
*/

  if(s->count < 0){
  
    //s->count = -1;
    tcb *tempSem;
    tempSem = s->q;

     if(tempSem == NULL){
      //printf("In heere\n");
      s->q = running;
     }
     else{
      int counter = 0;
        while(tempSem->next != NULL){
          //printf("%d In Q is %d\n",counter,tempSem->thread_id);
          //sleep(1);
          tempSem = tempSem->next;
          counter ++;
        }
      //printf("%d In Q is %d\n",counter,tempSem->thread_id);
      tempSem->next = running;
    }


    tcb *temp;
    temp = running;
    temp->next = NULL;

    if(ready != NULL){
      temp = running;
      running = ready;
      if(ready->next != NULL){
        ready = ready->next;
      }
      else{
        ready = temp;
      }
      running->next=NULL;
      swapcontext(temp->thread_context,running->thread_context);
    }
    else {
        printf("no other thread is available to run... program ends\n");
        exit(1);
    }
    //printf("Running Thread %d\n",running->thread_id);
     
  }
  //printf("S Count In wait End: %d\n",s->count);
}

void sem_signal(sem_t *s)
{

  s->count = s->count + 1;
 
  tcb* temp ;
  temp = ready;
if(temp != NULL){
  
  while(temp->next != NULL){
    //printf("READY is %d\n",temp->thread_id);
    temp = temp->next; 
  }   //Getting to the last section of ready

  if(s->q != NULL){
    tcb* temp2 = s->q->next;  //Gets the second in q of the semaphonre
    //printf("Poopscadoop\n");
    //printf("s->q: %d\n",s->q->thread_id);
    fflush(stdout);
    temp->next = s->q;   //Makes the last of the ready queue equal to s->q
    //printf("temp->next %d",temp->next->thread_id);
    temp->next->next =NULL; //Makes the last of the ready queues next equal to null
    s->q = temp2; //Makes s->q->next the new head of s
    //printf("New Last REady:%d \n",temp->thread_id);
    //printf("Ready %d\n",ready->next->thread_id);
  }
  
  else{
    //printf("Its null");
  }
  //printf("New Last REady:%d \n",temp->thread_id);
  //// printf("Ready %d\n",ready->thread_id);
  //t_yield();
  //printf("S Count In Singal End: %d\n",s->count);
}
}
void sem_destroy(sem_t **s)
{
  free(*s);
}

int mbox_create(mbox **mb){
  *mb = malloc (sizeof(mbox));
  return 1;
}

void mbox_deposit (mbox *mb, char *msg,int len){

}

void mbox_withdraw(mbox *mb, char *msg, int *len){

}

void mbox_destroy(mbox **mb){

}