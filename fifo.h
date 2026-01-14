#ifndef FIFO_H
#define FIFO_H
#include <pthread.h>
#include <semaphore.h>

#define MAX_QUEUE 100

typedef struct
{
    int buffer[MAX_QUEUE];
    int front;
    int rear;
    int size;
    pthread_mutex_t lock;
    sem_t semItems;
    sem_t semSlots;
} FifoQueue;

void fifoInit(FifoQueue *q);
void fifoDestroy(FifoQueue *q);
void fifoPush(FifoQueue *q, int val);
int fifoPop(FifoQueue *q);
int fifoIsEmpty(FifoQueue *q);
int fifoSize(FifoQueue *q);

#endif