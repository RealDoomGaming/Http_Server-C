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

// now the thread struct we will use
// We use a struct and not just a thread because we can contoll it better and //
// access it via and id
typedef struct thread {
  int id;                   // the id for our thread
  pthread_t pthread;        // our actual thread
  struct thpool_ *thpool_p; // the thread pool our thread belongs too
} thread;

// our thread pool for the thread struct later
typedef struct thpool_ {
  thread **threads; // a pointer to the threads
  volatile int numThreadsAlive;
  volatile int numThreadsWorking;
  pthread_mutex_t thcountLock;
  pthread_cond_t threadAllIdle;
  jobqueue jobqueue;
} thpool_;
