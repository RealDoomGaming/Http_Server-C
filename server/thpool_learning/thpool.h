// this file is the h file for our future thread pool c file
// and has all the necessary function and other stuff predefined

#ifndef _THPOOL_
#define _THPOOL_

#include <threads.h>
typedef struct thpool_ *threadpool;

// this here inits the threadpool and wont return at all until all the thread
// workers have been initizialized succesfully the param is the number of
// threads which will be created in this function and we return the created
// threadpool on success, if there is an error it will be NULL
threadpool thpoolInit(int numOfThreads);

// this function adds work to the work queue
// it takes an action and adds it the threadpools created job queue
// if we really wanted to add a function with more then one argument then a way
// of that is to pass a pointer of a structure with the function and all the
// arguments
// also we must cast both the funtion and the argument in order not to get any
// warnings
// argP is the argument for the funtion
// void (*functionP)(void*) -> means that the function returns void
// *functionP is just the pointer to the function and void * means that the
// function takes void* as an argument
int thpoolAddWork(threadpool, void (*functionP)(void *), void *argP);

// with this function we wait for all jobs in the queue to finish
// so when everything is done the main programm can actually continue to run as
// normal we have a polling which is initially 0 - meaning there is no polling
// at all If after 1 seconds the threads havent finished the polling grows
// exponentially until it reaches max seconds.
void thpoolWait(threadpool);

// this function just pauses all the worker threads immediately
// The threads will pause no matter if the are working or not and after
// calling thpool resume they return to the state before
void thpoolPause(threadpool);

// this function resumes all threads after pausing them with thpoolPause
void thoolResume(threadpool);

// this function just destroys all the threads no matter which state they are in
// this just kills all of them
void thpoolDestroy(threadpool);

// this function returns the current working threads
// Working threads are the threads which are currently working and no idle
int thpoolNumThreadsWorking(threadpool);

// this function will resume the "paused" threads
void thpoolResume(threadpool);

#endif
