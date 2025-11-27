#ifndef QUEUE
#define QUEUE

#include "node.h"
#include <stdlib.h>

typedef struct Queue {
  int len;
  node *firstE;
  node *lastE;
} queue;

queue *queueInit();
int enqueue(queue *queueP, node *nodeToQeueu);
node *dequeue(queue *queueP);
node *peek(queue *queueP);
int queueLength(queue *queueP);

#endif // !QUEUE
