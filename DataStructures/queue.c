
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
  // this is a function for enqueing new thins into the queue
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

node *dequeue(queue *queueP) {
  // this is a function for taking the first thing from our queue
  // firstly check if our queueP we got is valid
  if (queueP == NULL) {
    perror("Dequeue");
    printf("There was an error dequeueing because the queue we got was NULL\n");
    return NULL;
  }
  if (queueP->firstE == NULL) {
    perror("Dequeue");
    printf("There was an error dequeueing because the first element of the "
           "queue was NULL\n");
    return NULL;
  }

  // then if it is we can try to get the first thing out of the queue
  // we need to get the first thing out of the queue and set the first thing
  // after that to the thing after the first thing we pulled
  node *removed = queueP->firstE;

  // if we succesfully set a pointer to the first node then we can continue else
  // we do error stuff
  if (removed == NULL) {
    perror("Dequeue");
    printf("There was an error trying to dequeue the node because we couldnt "
           "get the first node\n");
    return NULL;
  }
  // if no error then we set the new pointer to the next node of the first node
  // before we repoint the pointer we have to check if len == 1 and then remove
  if (queueP->len == 1) {
    // first and last element of the queue
    queueP->firstE = NULL;
    queueP->lastE = NULL;
  } else {
    // else we do the normmal stuff
    queueP->firstE = queueP->firstE->next;
  }

  // however in all circumstances we have to remove 1 from the length
  queueP->len--;

  // if we can get it then we return it
  return removed;
}
