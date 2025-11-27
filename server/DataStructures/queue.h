#ifndef QUEUE
#include <stdlib.h>

typedef struct Queue {
  int len;
  void *firstE;
  void *lastE;
} queue;

static queue *queueInit();
static int *enqueue(queue *queueP, void *thingToQeueu);
static void *dequeue(queue *queueP);
static void *peek(queue *queueP);
static int queueLength(queue *queueP);

#endif // !QUEUE
