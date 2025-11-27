
#include "queue.h"
#include <stdio.h>

static queue *queueInit() {
  // this function just makes a new queue and with that also does the memory
  // allocation stuff
  queue *newQueue = (queue *)malloc(sizeof(queue));

  // we then check if there was an error allocating the memory for our queue
  if (newQueue == NULL) {
    perror("Queue Init");
    printf("There was an error trying to allocate the memory for the queue\n");
    return NULL;
  }

  // but then also does est the length and every other attribute to 0 or NULL;
  newQueue->len = 0;
  newQueue->firstE = NULL;
  newQueue->lastE = NULL;

  // lastly after that we can just return the queue pointer
  return newQueue;
}

static int *enqueue(queue *queueP, void *thingToQueue) {
  // check if thing to queue is valid and if we have a valid queueP
  // then we also have to check how many objects are already in the queue to do
  // different things and lastly just return the dequeued object
}
