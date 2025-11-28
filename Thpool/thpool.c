#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "../DataStructures/node.h"
#include "../DataStructures/queue.h"
#include "thpool.h"

threadPool *threadPoolInit(int numThreads) {
  // firstly we have to check if the numThreads is bigger then 0 else it wont
  // really make sense to make a Thread Pool with 0 threads in it
  if (numThreads <= 0) {
    perror("ThreadPool Init");
    printf("There was an error trying to init the thread pool because the "
           "given number of threads was smaller then 1\n");
    return NULL;
  }

  // after we know that we have a valid amount of threads we want to create we
  // can actually create the thread pool
  threadPool *thpool = (threadPool *)malloc(sizeof(threadPool *));
  // then we have to check if allocating the memory worked
  if (thpool == NULL) {
    perror("ThreadPool Init");
    printf(
        "There was an error trying to init the thread pool because we couldnt "
        "allocate the needed amount of memeory for the new ThreadPool\n");
    return NULL;
  }

  // after that we can init the less important things like the active and the
  // mutex and cond
  thpool->active = 1;
  pthread_mutex_init(&thpool->mutex, NULL);
  pthread_cond_init(&thpool->signal, NULL);

  // now after creating all those other params we can finally init the queue and
  // the pool of the threads
  queue *newQueue = queueInit();
  thpool->threadQueue = newQueue;

  // after making the queue we can now finally init the actuall pool with all of
  // our threads in it

  return thpool;
}

threadJob *threadJobInit(void *(*job)(void *), void *arg) {}
