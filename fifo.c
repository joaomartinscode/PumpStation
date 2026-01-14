#include "fifo.h"

void fifoInit(FifoQueue *q)
{
    q->front = 0;
    q->rear = 0;
    q->size = 0;
    pthread_mutex_init(&q->lock, NULL);
    sem_init(&q->semItems, 1, 0);
    sem_init(&q->semSlots, 1, MAX_QUEUE);
}

void fifoDestroy(FifoQueue *q)
{
    pthread_mutex_destroy(&q->lock);
    sem_destroy(&q->semItems);
    sem_destroy(&q->semSlots);
}

void fifoPush(FifoQueue *q, int val)
{
    sem_wait(&q->semSlots);
    pthread_mutex_lock(&q->lock);

    q->buffer[q->rear] = val;
    q->rear = (q->rear + 1) % MAX_QUEUE;
    q->size++;

    pthread_mutex_unlock(&q->lock);
    sem_post(&q->semItems);
}

int fifoPop(FifoQueue *q)
{
    sem_wait(&q->semItems);
    pthread_mutex_lock(&q->lock);

    int val = q->buffer[q->front];
    q->front = (q->front + 1) % MAX_QUEUE;
    q->size--;

    pthread_mutex_unlock(&q->lock);
    sem_post(&q->semSlots);
    return val;
}

int fifoIsEmpty(FifoQueue *q)
{
    pthread_mutex_lock(&q->lock);
    int empty = (q->size == 0);
    pthread_mutex_unlock(&q->lock);
    return empty;
}

int fifoSize(FifoQueue *q)
{
    pthread_mutex_lock(&q->lock);
    int s = q->size;
    pthread_mutex_unlock(&q->lock);
    return s;
}