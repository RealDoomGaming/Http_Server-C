
#include "queue.h"
#include "node.h"
#include <stdio.h>

queue *queueInit() {
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

int enqueue(queue *queueP, node *thingToQueue) {
  // check if thing to queue is valid and if we have a valid queueP
  if (queueP == NULL) {
    perror("Enqueue");
    printf("There was an error trying to enqueue because the queue given was "
           "NULL\n");
    return -1;
  }

  if (thingToQueue == NULL) {
    perror("Enqueue");
    printf("There was an error trying to enqueue because the given thing to "
           "queue was NULL\n");
    return -1;
  }
  // then we also have to check how many objects are already in the queue to do
  // different things
  if (queueP->len == 0) {
    queueP->firstE = thingToQueue;
    queueP->lastE = thingToQueue;
  } else {
    // setting the last thing to the new thing and then setting the next of the
    // last thing to the new thing
    queueP->lastE->next = thingToQueue;
    queueP->lastE = thingToQueue;
  }

  queueP->len++;

  // and lastly just return 0 if it all worked
  return 0;
}
