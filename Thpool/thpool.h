#ifndef THPOOL
#define THPOOL

#include "../DataStructures/queue.h"
#include <pthread.h>

typedef struct ThreadPool {
  // struct for the thread pool with the number of threads, then if its active,
  // then the queue where the jobs go then the actual array of threads then the
  // mutex which basically acts like a lock for the threads and the condition is
  // so we can reduce the impact of our threads on the cpu, the signal just
  // allows the thread to remain idle until it is signaled to continue
  int numThreads;
  int active;
  queue *jobQueue;
  pthread_t **pool;
  pthread_mutex_t mutex;
  pthread_cond_t signal;
  int signal_set;
} threadPool;

typedef struct ThreadJob {
  // for this struct we need a void pointer function with a void pointer return
  // and a void pointer arg and then we also need the argument itself in this
  // struct which ofc is going to be a void pointer arg
  void *(*job)(void *);
  void *arg;
} threadJob;

int addJob(threadPool *thpool, threadJob *thjob);
threadPool *threadPoolInit(int numThreads);
threadJob *threadJobInit(void *(*job)(void *), void *arg);

#endif // !THPOOL
