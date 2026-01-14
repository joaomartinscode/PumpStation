#include "pump.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define exit_on_error(s, m) \
    if (s < 0)              \
    {                       \
        perror(m);          \
        exit(1);            \
    }
#define exit_on_null(s, m) \
    if (s == NULL)         \
    {                      \
        perror(m);         \
        exit(1);           \
    }

void initPumps(PumpStation *ps, int count)
{
    ps->count = count;

    int shmP = shmget(IPC_PRIVATE, sizeof(Pump) * count, IPC_CREAT | 0666);
    exit_on_error(shmP, "Creation");
    ps->pumps = (Pump *)shmat(shmP, NULL, 0);
    exit_on_null(ps->pumps, "Attach");
    ps->shmIdP = shmP;

    int shmQ = shmget(IPC_PRIVATE, sizeof(FifoQueue) * count, IPC_CREAT | 0666);
    exit_on_error(shmQ, "Creation");
    ps->queue = (FifoQueue *)shmat(shmQ, NULL, 0);
    exit_on_null(ps->queue, "Attach");
    ps->shmIdQ = shmQ;

    for (int i = 0; i < count; i++)
    {
        ps->pumps[i].occupied = FREE;
        ps->pumps[i].clientId = -1;
        pthread_mutex_init(&ps->pumps[i].lock, NULL);
        pthread_mutex_init(&ps->pumps[i].statsLock, NULL);

        for (int j = 0; j < 3; j++)
            ps->pumps[i].liters[j] = 0;

        fifoInit(&ps->queue[i]);
    }
}

void destroyPumps(PumpStation *ps)
{
    for (int i = 0; i < ps->count; i++)
    {
        pthread_mutex_destroy(&ps->pumps[i].lock);
        pthread_mutex_destroy(&ps->pumps[i].statsLock);
        fifoDestroy(&ps->queue[i]);
    }
    shmdt(ps->pumps);
    shmdt(ps->queue);
    shmctl(ps->shmIdP, IPC_RMID, NULL);
    shmctl(ps->shmIdQ, IPC_RMID, NULL);
}

int assignPump(PumpStation *ps, int cid)
{
    for (int i = 0; i < ps->count; i++)
    {
        pthread_mutex_lock(&ps->pumps[i].lock);
        if (!ps->pumps[i].occupied)
        {
            ps->pumps[i].occupied = OCCUPIED;
            ps->pumps[i].clientId = cid;
            pthread_mutex_unlock(&ps->pumps[i].lock);
            return i;
        }
        pthread_mutex_unlock(&ps->pumps[i].lock);
    }
    return -1;
}

void releasePump(PumpStation *ps, int id)
{
    if (id < 0 || id >= ps->count)
        return;
    pthread_mutex_lock(&ps->pumps[id].lock);
    ps->pumps[id].occupied = FREE;
    ps->pumps[id].clientId = -1;
    pthread_mutex_unlock(&ps->pumps[id].lock);
}