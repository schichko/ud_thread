#include "t_lib.h"

struct tcb  //The struct for tcb
{
  int thread_id;  //The id of the thread
  int thread_priority;  //The priority of the thread, not really used
  ucontext_t *thread_context; //Context of the thread
  struct tcb *next; //Next thread 

};

typedef struct tcb tcb; //Actually defining the struct
tcb *running; //Definition of running thread
tcb *ready; //Definition of ready thread queue


typedef struct {  //semaphore structure
       int count; //The count of the semaphore
       tcb *q;  //The q
} sem_t;

struct messageNode{ //Structure of messageNode
  char *message;  //message of the node
  int len;  //length of the message
  int sender; //sender thread
  int receiver; //reciever thread 
  struct  messageNode *next;  //the next message in the queue
};

typedef struct messageNode messageNode; //Definition of the structure of the message node

typedef struct //structure of the mailbox
{
  struct messageNode *msg;  //contains a mail thread
  sem_t *mbox_sem;  //And contains a semaphore that will be used for sending and recieving 
}mbox;  

mbox *myMbox; //Creating a mailbox we will use for sending and receiving 


void t_yield()  //Stops a thread
{
  //printf("In Yeild\n");
  tcb *temp;  //Creates a temp
  temp = running; //assigns temp to the current running thread so we can do a switch later on
  temp->next = NULL;  //makes temp->next null because running should never have a next thread
  //printf("Ready:%d\n",ready->thread_id);
  if(ready != NULL){  //if ready has more to do
    running = ready;  //We make running the first of ready
    ready = ready->next;  //And we move ready down to the next thread in the ready q
    running->next=NULL; //makes running (the previous ready) next = to null because as said before it is never supposed to have a next
    tcb *temp2; //Makes a new temp that will be used for switching
    temp2 = ready;  //temp2 is the first ready
    if(temp2 == NULL){  //if there is no new ready we make ready=to temp the old running
      ready = temp;
    }
    else{ //Otherwise we put it at the end of the ready q
      while(temp2->next != NULL){ //we move to the end
        temp2 = temp2->next;
      }
      temp2->next = temp; //and insert
    }
  }
  //printf("Running:%d\n",running->thread_id);
  swapcontext(temp->thread_context,running->thread_context); //We swap the context to make the other thread resume
}

void t_init() //function for initilizing the threads
{
  tcb *temp = (tcb*)(malloc(sizeof(tcb)));  //Malloc space for tcb
  temp->thread_context = (ucontext_t *) malloc(sizeof(ucontext_t)); //Give it context
  getcontext(temp->thread_context); //Get context
  temp->next = NULL;  //There is no next 
  temp->thread_id = 0;  //Initial thread
  running = temp; //It is running
  ready = NULL; //Nothing is ready
}

int t_create(void (*fct)(int), int id, int pri) //Function for making new threads
{
  size_t sz = 0x10000;  

  tcb *temp = (tcb*)(malloc(sizeof(tcb)));//malloc size of tcb
  temp->thread_context = (ucontext_t *) malloc(sizeof(ucontext_t)); //Give it context similar to for init
  getcontext(temp->thread_context); //Get that context
  temp->thread_id = pri;  //Assign the thread priority passed in through parameters
  temp->thread_id = id; //Assign the thread id also passed in through parameters 
  temp->next = NULL;  //Thread originally has no null

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
  if(ready == NULL){  //We are inserting this thread into the ready que, so if the q is empty we make it the first spot
    ready = temp;
  }
  else{ //Otherwise we loop to the end and then assign it to the end
    tcb* looper = ready;
    while(looper->next != NULL){  //Loops through to the end
      looper = looper->next;
    }
    looper->next = temp;
  }
}

void t_shutdown(void){  //Shuts down the thread
  if(ready != NULL){  //If we dont have any ready threads then we ignore this 
    tcb *temp ; //Other wise we need to make a temp for looping
    while(ready != NULL){ //We want to free one
      temp = ready->next;
      free(temp->thread_context->uc_stack.ss_sp); //Free the parts of the thread
      free (ready->thread_context);
      free(ready);
      ready = temp;
    }
  }
  free(running->thread_context);  //Free the parts of running
  free(running);
}

void t_terminate(){ //Terminates
  tcb* temp;  //Used for reassigning
  temp = running; //Saves the running thread
  running = ready;  //Running is now the first in the ready q
  if(ready != NULL){  //We reassign the ready q
    ready = ready->next;
  }
  free(temp->thread_context->uc_stack.ss_sp); //We terminate the old running thread and free stuff in it
  free (temp->thread_context);  //We free the context
  free(temp); //We free the thread
  setcontext(running->thread_context);  //And set the context to the new running
}


int sem_init(sem_t **sp, int sem_count)  //Our inital code for the sem
{
  *sp = malloc(sizeof(sem_t));  //Malloc and make space for the semaphore
  (*sp)->count = sem_count; //Set the count for the semaphore
  (*sp)->q = NULL;  //No q
}

void sem_wait(sem_t *s) //wait function, if a thread calls this and cannot get in, it switches out context
{

 // printf("S Count In wait Start: %d\n",s->count);
  s->count = s->count -1; //Decrement semahphore

/* Put this here to see what needed to be done
For semaphore operations, if sem_wait() decides that the calling
thread needs to block itself, it will
(1) remove the TCB of the "calling" thread from the RUNNING
    queue and insert it to the end of this semaphore's queue,
(2) promotes the first TCB of the READY queue into the RUNNING
queue, and
(3) calls swapcontext().
*/

  if(s->count < 0){ //Check to see if we have made count an illegal value
  
    //s->count = -1;
    tcb *tempSem; //Temp sem 
    tempSem = s->q; //Based on the sem s, get the value of q

     if(tempSem == NULL){ //If we do not have anything in the q we want running 
      //printf("In heere\n");
      s->q = running; //the running thread is now in the semaphores q for being woken up
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


    tcb *temp;  //Temp thread for moving things around
    temp = running; //Temp is set to the current running threads
    temp->next = NULL;    //Temp->next (AKA running ->next) should be null

    if(ready != NULL){  //If we have more things avail inside of the ready q we must set their context
      temp = running; //shouldve already been done but saftey check
      running = ready;  //Running is now the first one in ready
      if(ready->next != NULL){  //reassign ready
        ready = ready->next;
      }
      else{
        ready = temp; //Reasign ready
      }
      running->next=NULL;
      swapcontext(temp->thread_context,running->thread_context);  //Swap the context like in yeild
    }
    else {  //We dont have anything in the ready q
        printf("no other thread is available to run... program ends\n");
        exit(1);
    }
    //printf("Running Thread %d\n",running->thread_id);
     
  }
  //printf("S Count In wait End: %d\n",s->count);
}

void sem_signal(sem_t *s) //Singal the semaphore to increase count by 1, if we do this there should be some more space in the sem
{

  s->count = s->count + 1;  //Actually increase the count of the semaphore
 
  tcb* temp ; //temp variable
  temp = ready; //temp is ready
if(temp != NULL){ //
  
  while(temp->next != NULL){  //loop
    //printf("READY is %d\n",temp->thread_id);
    temp = temp->next; 
  }   //Getting to the last section of ready

  if(s->q != NULL){
    tcb* temp2 = s->q->next;  //Gets the second in q of the semaphonre
    //printf("s->q: %d\n",s->q->thread_id);
    fflush(stdout);
    temp->next = s->q;   //Makes the last of the ready queue equal to s->q
    //printf("temp->next %d",temp->next->thread_id);
    temp->next->next =NULL; //Makes the last of the ready queues next equal to null
    s->q = temp2; //Makes s->q->next the new head of s
    //printf("New Last REady:%d \n",temp->thread_id);
    //printf("Ready %d\n",ready->next->thread_id);
  }
  
  else{ //If this isnt true we do nothing
    //printf("Its null");
  }
  //printf("New Last REady:%d \n",temp->thread_id);
  //// printf("Ready %d\n",ready->thread_id);
  //t_yield();
  //printf("S Count In Singal End: %d\n",s->count);
}
}
void sem_destroy(sem_t **s) //Frees the semaphore that was malloced earlier
{
  free(*s);
}

int mbox_create(mbox **mb){ //Creates the mailbox
  *mb = malloc (sizeof(mbox));
  // myMbox = malloc(sizeof(mbox));
  return 1;
}

void mbox_deposit (mbox *mb, char *msg,int len){  //Used for depositing a message into a certain mailbox
  char *newString = (char *) malloc(len*sizeof(char));  //Mallocs size for a new string that we can copy
  strcpy(newString,msg);  //Copys the message into newstring 
  messageNode *newMSG = (messageNode*)(malloc(sizeof(messageNode)));  //Creates a new message node
  newMSG->message = newString;  //assigns the message to the passed in message
  newMSG->len = len;  //And its length to the passed in length
  newMSG->sender = 0; //No specified sender 
  newMSG->receiver = 0; //No sepcified reciever
  newMSG->next = NULL;  //No next at the moment


  messageNode *temp;  //Used for looping though the mailboxes current q
  temp = mb->msg; //Assigned to first part of the mailboxes message q
  if(temp == NULL){ //If the mailbox is empty we make the message equal to the first mailbox message
    mb->msg = newMSG;
  }
  else  //Else we have more messages to go thorugh
  {
    while(temp->next != NULL){
      temp = temp->next;  //Loop through
    }
    temp->next = newMSG;  //Assign the end of the loop to the new message
  }
}

void mbox_withdraw(mbox *mb, char *msg, int *len){  //The code for withdrawing from a mailbox
  messageNode *temp;  //The temporary message node
  temp = mb->msg; //temp is equal to the first pointer in the mailbox
  messageNode *temp2; //A second temp varaible used for swapping things
  if(temp != NULL){ //If we have something in the mailbox
    if(temp->next == NULL){ //If there is no next
      *len = 0;
    }
    else{ //Else we have to rearange some stuff
      temp2 = temp->next; //W
      mb->msg = temp2;
      temp->next = NULL;
    }
    

    if(temp == NULL){ //mailbox is empty
      len = 0;
    }
    else{
      fflush(stdout); //fflush used for testing


      strcpy(msg,temp->message);  //Otherwise we move the message over
      *len = (temp->len); //Move the length over
      free(temp); //And free the temp variable
    }
  }
}

void send(int tid, char *msg, int len){ //Used for interthread communication
  if(myMbox == NULL){ //If this is the first send
    mbox_create(&myMbox); //Creates the mbox I used a special mailbox that stores all of the inter thread communication
    sem_init(&myMbox->mbox_sem,0);  //initilizes the semaphore for the mbox
  }
  
  char *newString = (char *) malloc(len*sizeof(char) +1); //We allocate space for the string similar to in deposit 
  strcpy(newString,msg);  //we copy the string over
  messageNode *newMSG = (messageNode*)(malloc(sizeof(messageNode)));  //And allocate spaec of a new message node struct like before
  newMSG->message = newString;  //String movement 
  newMSG->len = len;  //Length copying
  newMSG->sender = running->thread_id;  //We want the sender to be the current running thread
  newMSG->receiver = tid; //The receive variable has been specified inside of 
  newMSG->next = NULL;  //No next currently 

  messageNode *temp;  //Temporary variable that will be used for switching

  temp = myMbox->msg; //Set that temp variable equal to the firts message in the mailbox

  if(temp == NULL){ //If there is nothing in the mailbox
    myMbox->msg = newMSG; //We have the first message in the mailbox be equal to this new variable
  }
  else{ //There was something in the mailbox
    while(temp->next != NULL){  //We traverse though the mailbox
      temp = temp->next;
    }
    temp->next = newMSG;  //And add to the end of the mailbox
  }

  sem_signal(myMbox->mbox_sem); //Let the semaphore know that we have more material
}

void receive(int *tid, char *msg, int *len){  //Used for receiving a message
  if(myMbox->msg == NULL){  //If the mailbox is empty we dont have any message
    // printf("There are no messages to withdraw from the\n");
  }
  else{
    sem_wait(myMbox->mbox_sem); //We wait, if we dont have any messages we will wait, because sem is init as 0
    
  
    if(myMbox == NULL){ //If this is the first recv or send call or by some chance the mbbox is null we want to init
      mbox_create(&myMbox); //Create
      sem_init(&myMbox->mbox_sem,0);
    }

    //tid is the sender it wants to recieve from 0 if anyone
    

    //Theres a message
    messageNode *temp;  //temp variable used for switching later
    temp = myMbox->msg; //set the message equal to the first message avaible 
    messageNode *lastTemp;  //Used for reassignment
    // printf("Error past here\n");

    if(temp == NULL){ //This should never happen at this point
      printf("Something is wrong\n");
    }
    else{
      int off = 0;  //Used for saying we found a message

      //This is checking the first message
      if((temp->sender == *tid || *tid==0 ) && (temp->receiver == running->thread_id)) {  //We must have a message that is who we want to recievr from or 0, and the message needs to be sent to the running thread
        // printf ("WE NEED\n");
        // printf("MESSAGE :%s\n",temp->message);
        strcpy(msg,temp->message);  //Copy the string over
        //  printf("Error past here7\n");
        *tid = temp->sender;  //The tid is the tender that we have recieved 
        *len = temp->len; //The length is the given length
        off =1; //We are switching off because we have recieved the message
        if(lastTemp == NULL){
          myMbox->msg = temp->next;
          free(temp);
        }
        else{
          myMbox->msg = temp->next;
          free(temp);
        }
      }
      //This is checking the other messages
      while(temp->next != NULL && off==0){  
        //printf("WE LOOP");
        if((temp->sender == *tid || *tid==0 ) &&(temp->receiver == running->thread_id)) { //We must have a message that is who we want to recievr from or 0, and the message needs to be sent to the running thread
          //printf ("WE NEED\n");
          strcpy(msg,temp->message); //Copy the string over
          *tid = temp->sender; //The tid is the tender that we have recieved 
          *len = temp->len;//The length is the given length
          off =1;//We are switching off because we have recieved the message 
          if(lastTemp == NULL){
            myMbox->msg = temp->next;
            free(temp); //Free the message, no longer needed
          }
          else{
            lastTemp->next = temp->next;
            free(temp);//Free the message, no longer needed
          }
        }
        else{ //Reasign 
          lastTemp = temp; //last temp is our current node
          temp = temp->next;  //We move on our current node
          
        }
        
      }
    }
  }
}

void mbox_destroy(mbox **mb){ //We destroy the mailbox
  messageNode *temp; 
  free(*mb);//We free the mailbox, all of hte mail should have already been freed
}