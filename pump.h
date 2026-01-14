#ifndef PUMP_H
#define PUMP_H
#include <pthread.h>
#include "fifo.h"

#define FREE 0
#define OCCUPIED 1

typedef struct
{
    int occupied;
    int clientId;
    int liters[3];
    pthread_mutex_t lock;
    pthread_mutex_t statsLock;
} Pump;

typedef struct
{
    Pump *pumps;
    FifoQueue *queue;
    int count;
    int shmIdP;
    int shmIdQ;
} PumpStation;

void initPumps(PumpStation *ps, int count);
void destroyPumps(PumpStation *ps);
int assignPump(PumpStation *ps, int cid);
void releasePump(PumpStation *ps, int id);

#endif