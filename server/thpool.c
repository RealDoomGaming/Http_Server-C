// our header file for the functions we need in our main c file
#include "thpool.h"

// all other imports
#include <bits/pthreadtypes.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <time.h>
#include <unistd.h>

// here come our structs
// Binary Semaphore
// Acts like a tiny stop and go flag for our worker threads with
// 0 -> threads must wait
// 1 -> threads may proceed
typedef struct bsem {
  pthread_mutex_t
      mutex; // this ensures that only one thread can manipulate the
             // semaphores value at one time because if multiple threads
             // tried changing v at the same time that would be VERY bad
  pthread_cond_t cond; // conditional variable which lets a thread go eep while
                       // waiting for the semaphore to become available again
                       // and wakes it  up once someone signals it
  int v;               // actual value, can only be 0 and 1
} bsem;

// firstly the struct for our job
typedef struct job {
  struct job *prev;            // pointer to our previous job
  void (*function)(void *arg); // then a pointer to a function
  void *arg;                   // and finally our functions arguments
} job;

// secondly our job queue struct
typedef struct jobqueue {
  pthread_mutex_t rwmutex; // this is for the read and write access of our jobs
  job *front;    // then we have a pointer to the first object in the queue
  job *rear;     // also a pointer to the second object in the queue
  bsem *hasJobs; // bsem is shorthand for binary semaphore, explained when we
                 // define it
  int len;       // finally the number of jobs in the queue
} jobqueue;

// thrid our struct for our actual thread
typedef struct thread {
  int id;                  // id so we can easily identify each thread
  pthread_t pthread;       // actual thread which will work
  struct thpool_ *thpoolP; // the threads access to the threadpool
} thread;

// our thread pool with which we will actually work
typedef struct thpool_ {
  thread **threads;               // the pointer to all the threads
  volatile int numThreadsAlive;   // the number of threads currently alive
  volatile int numThreadsWorking; // the number of threads currently working
  pthread_mutex_t
      thcountLock; // this will be used for thread count and other stuff
  pthread_cond_t threadsAllIdle; // would signal to all threads to wait with
                                 // thpool wait func
  jobqueue jobqueue;             // our job queue with all the jobs
} thpool_;

// now our function prototypes -> function prototypes are basically when we tell
// the c compiler that we will later declare the functions but for now you just
// need to know their names arguments and return types
//
// Why do we need them -> because C is a top to bottom language meaning it reads
// everything in order from top to bottom and if we place the functions at the
// top then any part of the file can call them

// our funtions for working with our threads
static int threadInit(thpool_ *thpoolP, struct thread **threadP, int id);
static void *threadDo(struct thread *threadP);
static void threadHold(int sigId);
static void threadDestroy(struct thread *threadP);

// our functions for working with out jobqueue
static int jobqueueCreate(jobqueue *jobqueueP);
static void jobqueueClear(jobqueue *jobqueueP);
static void jobqueueInsert(jobqueue *jobqueueP, struct job *newJobP);
static struct job *jobqueueGet(jobqueue *jobqueueP);
static void jobqueueDestroy(jobqueue *jobqueueP);

// our function for the bsem
static void bsemCreate(struct bsem *bsemP, int value);
static void bsemReset(struct bsem *bsemP);
static void bsemPost(struct bsem *bsemP);
static void bsemPostAll(struct bsem *bsemP);
static void bsemWait(struct bsem bsemP);

// our main stuff for the actual threadpool now

// first we need to create/init our thread pool
// the function only gets one parameter and that would be the num of Threads
// we want max
struct thpool_ *thpoolInit(int numThreads) {
  // error variable which will be used for handeling errors late
  int err = 0;

  // if numThreads is less then 0 so -1, -2 and so on we want to set it to 0
  if (numThreads < 0) {
    numThreads = 0;
  }

  // now to make a new thread pool
  thpool_ *thpoolP;
  // we need to malloc memory to our thread pool in the size of the threadpool
  // struct and then cast it to a threadpool pointer bc it isnt one
  // automatically
  thpoolP = (struct thpool_ *)malloc(sizeof(struct thpool_));

  // if the threadpool is still null after allocating memory then something bad
  // happened 100%
  if (thpoolP == NULL) {
    perror("allocating memory");
    printf("There was an error allocating memory for the initializing of your "
           "thread pool\n");
    return NULL;
  }
  // after we know that no error happened when creating the threadpool
  // we can initialize the number of threads working and alive in the struct
  thpoolP->numThreadsAlive = 0;
  thpoolP->numThreadsWorking = 0;

  // after all that we can initialize the actual jobqueue
  err = jobqueueCreate(&thpoolP->jobqueue);
  if (err == -1) {
    perror("Jobqueue creation");
    printf("There was an error creating the jobqueue for the jobs\n");
    free(thpoolP);
    return NULL;
  }

  // then we also need to make the threads in the pool
  thpoolP->threads =
      (struct thread **)malloc(numThreads * sizeof(struct thread *));
  // if we have an error, so meaning we couldnt malloc the memory we do our
  // error handeling
  if (thpoolP->threads == NULL) {
    perror("Thread Pool creation");
    printf("There was an error trying to malloc the memory for the threads in "
           "our Thread Pool\n");
    free(thpoolP);
    return NULL;
  }

  // here we just initialize the lock and the all idle signals for our thpool
  pthread_mutex_init(&(thpoolP->thcountLock), NULL);
  pthread_cond_init(&thpoolP->threadsAllIdle, NULL);

  // lastly we actually need to init the threads
  int n;
  // we go with a for loop for the amount of number of threads we give this
  // function
  for (n = 0; n < numThreads; n++) {
    // init the thread with our init funtion where we give it the thpool then
    // the thread and also the id
    threadInit(thpoolP, &thpoolP->threads[n], n);
  }

  // but we also have to wait for all the threads to initialize
  // meaning we wait until the amount of threads is the same as our number of
  // threads
  while (thpoolP->numThreadsAlive != numThreads) {
  };

  // lastly we return the initialized thpool
  return thpoolP;
}

// now we have to make the function for the work to be added to our thpool
int thpoolAddWork(thpool_ *thpoolP, void (*functionP)(void *), void *argP) {
  // a pointer to our new Job
  job *newJob;

  // initialize the new job
  newJob = (struct job *)malloc(sizeof(struct job));
  // if something with the malloc failed then we do our error handeling
  if (newJob == NULL) {
    perror("Creating Job");
    printf("There was an error allocating memor for our new job");
    return -1;
  }

  // after that we have to assign the function and the argument
  newJob->function = functionP;
  newJob->arg = argP;

  // lastly we have to add the job to the queue
  jobqueueInsert(&thpoolP->jobqueue, newJob);

  // after nothing went wrong we just return with 0
  return 0;
}

// next we have to make a function which waits for all jobs to finish
void thpoolWait(thpool_ *thpoolP) {
  // activates the lock signal of the thread pool
  pthread_mutex_lock(&thpoolP->thcountLock);

  // while our jobqueue is bigger then 0 (true) or while our threads which are
  // working is bigger then 0 (true) we do the wait and send the signal to lock
  // and go idle
  while (thpoolP->jobqueue.len || thpoolP->numThreadsWorking) {
    pthread_cond_wait(&thpoolP->threadsAllIdle, &thpoolP->thcountLock);
  }

  // then after allat is done we dont signal to lock anymore but they are still
  // all idle
  pthread_mutex_unlock(&thpoolP->thcountLock);
}
