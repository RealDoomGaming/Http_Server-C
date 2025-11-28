#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "../DataStructures/node.h"
#include "../DataStructures/queue.h"
#include "thpool.h"

int addJob(threadPool *thpool, threadJob *job) {
  // in this function we basically just want to check if both parameters are
  // valid and then add the job to the queue of the thread pool but we need a
  // node for the queue so we also need to make a new node into which the job
  // will be inserted into
  if (thpool == NULL) {
    perror("Add Job");
    printf("There was an error while trying to add a job to the queue because "
           "the thread pool was NULL\n");
    return -1;
  }
  if (job == NULL) {
    perror("Add Job");
    printf("There was an error while trying to add a job to the queue because "
           "the job was NULL\n");
    return -1;
  }

  // then after everything was checked to be valud we have to add a new node
  node *newNode = (node *)malloc(sizeof(node));
  // check for error
  if (newNode == NULL) {
    perror("Add Job");
    printf("There was an error trying to make a new node for our job because "
           "malloc failed\n");
    return -1;
  }
  // we only need to set the job for the newNode because the next variable in
  // the newNode will be handeled inside of enqueue but we should still set it
  // to NULL;
  newNode->data = job;
  newNode->next = NULL;

  // after setting and making the new Node we can enqueue it into our queue from
  // the thread pool
  int err = enqueue(thpool->jobQueue, newNode);
  if (err == -1) {
    perror("Add Job");
    printf("There was an error trying to enqueue the new node into our job "
           "queue, the function enqueue gave us an error\n");
    return -1;
  }

  // after all that we need to send a signal with our cond signal from our
  // thread pool to signal one of our threads to awaken and start working
  pthread_cond_signal(&thpool->signal);

  // at the end if everything went well we just return 0
  return 0;
}

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
  thpool->jobQueue = newQueue;

  // after making the queue we can now finally init the actuall pool with all of
  // our threads in it
  pthread_t **threadArray =
      (pthread_t **)malloc(numThreads * sizeof(pthread_t *));
  // we also need to check if the malloc was sucessfull
  if (threadArray == NULL) {
    perror("ThreadPool Init");
    printf("There was an error trying to init the thread Array for the thread "
           "pool\n");
    return NULL;
  }

  // we need to make a for loop so we can init all threads
  int i;
  for (i = 0; i < numThreads; i++) {
    // then we make a new thread for the amount of threads
    pthread_t *newThread = (pthread_t *)malloc(sizeof(pthread_t));
    // then assign in to our array of threads we made
    threadArray[i] = newThread;
  }
  // lastly we just assign the thread array to the thread array of our thread
  // pool
  thpool->pool = threadArray;

  // lastly we just return the thread pool because we are done
  return thpool;
}

threadJob *threadJobInit(void *(*job)(void *), void *arg) {
  // this function just makes a new thread job struct and gives it back
  threadJob *newThreadJob = (threadJob *)malloc(sizeof(threadJob));

  // checking if the allocation was successfull
  if (newThreadJob == NULL) {
    perror("ThreaJob Init");
    printf("There was an error trying to allocate memory for the new thread "
           "job\n");
    return NULL;
  }

  // here we just give the new thread job the arguments it needs
  newThreadJob->job = job;
  newThreadJob->arg = arg;

  // and then we return it
  return newThreadJob;
}
