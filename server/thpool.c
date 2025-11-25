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

// 2 global variables
static volatile int threadsKeepAlive; // this variable is either 0 or 1 and says
                                      // if we should continue or worker loop
static volatile int threadsOnHold;    // this variable tells our threads to wait
                                      // with either being 0 or 1

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
  volatile int maxNumberThreads;  // the max number of threads we want
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
  thpoolP->maxNumberThreads = numThreads;

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

// next we have to make the destroy function for the threadpool which destroys
// it
void thpoolDestroy(thpool_ *thpoolP) {
  // we dont need to destroy it if it is already NULL
  if (thpoolP == NULL) {
    return;
  }

  // we need a variable for all the threads which exist so we can
  // see how many exist even after we destroyed the thpool
  volatile int threadsTotal = thpoolP->numThreadsAlive;

  // we will give our idle threads 1 second to be killed
  double TIMEOUT = 1.0;
  // we need this to calculate the time it takes to kill each thread
  time_t start, end;
  // time passed bewteen start and end
  double tpassed = 0.0;
  // start our start timer
  time(&start);
  // while the tpassed variable is lower then the TIMEOUT variable and
  // we still have threads alive we keep killing them
  while (tpassed < TIMEOUT && thpoolP->numThreadsAlive) {
    // we tell all our thread with a job to stop working and lock them
    bsemPostAll(thpoolP->jobqueue.hasJobs);
    // get the time of the end
    time(&end);
    // then get the difference between the start and end
    tpassed = difftime(start, end);
  }

  // then we need to pool the remaining threads
  // meaning we kill the remaining threads but with no timeout so
  // it isnt as gentle as before
  // this is slower but more friendly to the cpu
  while (thpoolP->numThreadsAlive) {
    bsemPostAll(thpoolP->jobqueue.hasJobs);
    sleep(1);
  }

  // we need to also clean up the jobqueue
  // thankfully we have a function for that
  jobqueueDestroy(&thpoolP->jobqueue);
  // now at the ends we also want to deallocate all the threads
  int n = 0;
  for (n = 0; n < threadsTotal; n++) {
    threadDestroy(thpoolP->threads[n]);
  }
  // at the end we can free our thread pool and the threads which it contains
  free(thpoolP->threads);
  free(thpoolP);
}

// this function "pauses" all threads (it kills them)
void thpoolPause(thpool_ *thpoolP) {
  int n;
  for (n = 0; n < thpoolP->numThreadsAlive; n++) {
    // sends the signal to every single thread to tell them to kill themselfs
    pthread_kill(thpoolP->threads[n]->pthread, SIGUSR1);
  }
}

// this function resumes all the threads in the thread pool (it creates new
// ones)
void thpoolResume(thpool_ *thpoolP) {
  // we get the number of threads we want to revive
  int numberOfThreadsWeWant = thpoolP->maxNumberThreads;

  int n = 0;
  for (n = 0; n < numberOfThreadsWeWant; n++) {
    // then we go throug that amount of threads we want to revive and init them
    threadInit(thpoolP, &thpoolP->threads[n], n);
  }
}

int thpoolNumThreadsWorking(thpool_ *thpoolP) {
  return thpoolP->numThreadsWorking;
}

// now we will do all the functions referring to the threads

// this function is fro creating a thread
// we will return 0 on sucess else we return -1
static int threadInit(thpool_ *thpoolP, struct thread **threadP, int id) {
  // here we actually give our thread the memory it needs
  *threadP = (struct thread *)malloc(sizeof(struct thread));
  if (threadP == NULL) {
    perror("Creating Thread");
    printf("Error creating a new thread\n");
    return -1;
  }

  // then we give it the refrence to our thread pool
  (*threadP)->thpoolP = thpoolP;
  // and its id
  (*threadP)->id = id;

  // after all that we create a new thread with pthread_create
  // &(*threadP)->pthread : means that we get the actuall thread from our thread
  // struct which is ** so we need to do the * and & (void *(*)(void *))
  // threadDo : means that we take the threadDo function and cast it to a
  // pointer which will point to a function which returns a void pointer and
  // accepts the void pointer as an argument lastly as an argument for our new
  // function we give it the *threadP
  pthread_create(&(*threadP)->pthread, NULL, (void *(*)(void *))threadDo,
                 *threadP);
  // with this we tell the system that I wont join the thread using pthread_join
  // so the system automatically cleans up the resources after the thread
  // finishes
  pthread_detach((*threadP)->pthread);
  return 0;
}

// we also need to make a function which makes the threads get called to go on
// hold
static void threadHold(int sigId) {
  // this function basically just loops until something else changes the
  // threadOnHold so basically just holds the threat
  int threadOnHold = 1;
  while (threadOnHold) {
    sleep(1);
  }
}

// now a function for the thread to actually do something
// This function is an endless loop which basically does something for infinity
// It gets errupted once the thpoolDestroy() function gets called
// thread* threadP -> the thread which will actually do something
static void *threadDo(struct thread *threadP) {
  // we could give the thread a name with snprintf(...)
  // but I wont do that because I am lazy
  // and we would also need to use something like prctl(...)

  // we make a new threadpool which is equal to the thread pool in the thread we
  // want to do something with
  thpool_ *thpoolP = threadP->thpoolP;
  // this is necesarry in order to ensure that all threads have been created
  // before we start

  // we firstly need to register the signal handler for the thread
  struct sigaction act; // creates a new sigaction struct which defines how this
                        // thread should handle a signal
  sigemptyset(&act.sa_mask); // sigemptyset inits the mask of the signal which
                             // should be blocked while the handler runs
                             // sigemptyset means that no extra signal should be
                             // blocked during the execution of threadHold
  act.sa_flags = SA_ONSTACK; // This tells linux to use an alternative signal
                             // stack when its running on the handler
  act.sa_handler = threadHold; // this sets the function which will be called
                               // when the thread receives a SIGUSR1
  // This if basically looks if there was an error trying to install the handler
  // for the sigaction, the action on SIGUSR1 was calling threadDo
  if (sigaction(SIGUSR1, &act, NULL) == -1) {
    perror("Signal Handler");
    printf("There was an error trying to set the sig on action\n");
    return NULL;
  }

  // then we also need to mark the thread as alive
  // we basically say here that we lock the access to the variable to all other
  // threads then we increment it and then we unlock the access to the variable
  // again We do this so no 2 threads can access it at the same time because
  // when we create the threads they might try to access them at the same time
  // and if 2 threads see 0 and do +1 then the final result is 1 but there are 2
  // threads in total
  pthread_mutex_lock(&thpoolP->thcountLock);
  thpoolP->numThreadsAlive += 1;
  pthread_mutex_unlock(&thpoolP->thcountLock);

  // now we do the main worker loop
  // while the threads are supposed to be alive we keep doing the thread
  while (threadsKeepAlive) {
    // this basically waits for a job to be free
    // if there are no jobs -> the threads sleep
    // else if there are -> the producer calls bsem_post and this wakes exactly
    // 1 worker
    bsemWait(*thpoolP->jobqueue.hasJobs);

    // if we are supposed to keep doing jobs then we do them
    // we check this again because the shutdown might happen after we just
    // entered another loop
    if (threadsKeepAlive) {
      // basically the same stuff we did before with num threads alive
      pthread_mutex_lock(&thpoolP->thcountLock);
      thpoolP->numThreadsWorking += 1;
      pthread_mutex_unlock(&thpoolP->thcountLock);

      // now we need to read a job from the jobqueue
      // the first thing we need is a function pointer (pointer to a function)
      // with a void pointer as a parameter
      void (*funcBuffer)(void *);
      // we also have an argument void pointer
      void *argBuffer;
      // and finally we get our job from the jobqueue
      job *jobP = jobqueueGet(&thpoolP->jobqueue);
      // then if we have got a job
      if (jobP) {
        // we get our function pointer an actual value from the job
        funcBuffer = jobP->function;
        // and also get the argument
        argBuffer = jobP->arg;
        // then we call it
        funcBuffer(argBuffer);
        // and free the job
        free(jobP);
      }

      // after we are done working we want to lock the num threads working again
      pthread_mutex_lock(&thpoolP->thcountLock);
      // and decrease it by 1
      thpoolP->numThreadsWorking -= 1;
      if (!thpoolP->numThreadsWorking) {
        // now if no threads are working we just call threadAllIdle to wake up
        // all threads so they maybe find something to work wit idk I think
        // maybe
        pthread_cond_signal(&thpoolP->threadsAllIdle);
      }
      // and we unlock the num threads working again
      pthread_mutex_unlock(&thpoolP->thcountLock);
    }
  }

  // here we just decrease the amount of alive workers because after this the
  // thread ends and its will be gone
  pthread_mutex_lock(&thpoolP->thcountLock);
  thpoolP->numThreadsAlive--;
  pthread_mutex_unlock(&thpoolP->thcountLock);

  // maybe we should destroy it beforehand
  threadDestroy(threadP);

  return NULL;
}

// freeing a thread
static void threadDestroy(thread *threadP) { free(threadP); }
