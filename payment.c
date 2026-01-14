#include "payment.h"
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

void initTerms(PaymentStation *ps, int count)
{
    ps->count = count;

    int shmT = shmget(IPC_PRIVATE, sizeof(Terminal) * count, IPC_CREAT | 0666);
    exit_on_error(shmT, "Creation");
    ps->terms = (Terminal *)shmat(shmT, NULL, 0);
    exit_on_null(ps->terms, "Attach");
    ps->shmIdT = shmT;

    int shmQ = shmget(IPC_PRIVATE, sizeof(FifoQueue) * count, IPC_CREAT | 0666);
    exit_on_error(shmQ, "Creation");
    ps->queue = (FifoQueue *)shmat(shmQ, NULL, 0);
    exit_on_null(ps->queue, "Attach");
    ps->shmIdQ = shmQ;

    for (int i = 0; i < count; i++)
    {
        ps->terms[i].occupied = FREE;
        ps->terms[i].clientId = -1;
        pthread_mutex_init(&ps->terms[i].lock, NULL);
        pthread_mutex_init(&ps->terms[i].statsLock, NULL);

        ps->terms[i].totalPayments = 0;
        ps->terms[i].revenue = 0.0;

        fifoInit(&ps->queue[i]);
    }
}

void destroyTerms(PaymentStation *ps)
{
    for (int i = 0; i < ps->count; i++)
    {
        pthread_mutex_destroy(&ps->terms[i].lock);
        pthread_mutex_destroy(&ps->terms[i].statsLock);
        fifoDestroy(&ps->queue[i]);
    }
    shmdt(ps->terms);
    shmdt(ps->queue);
    shmctl(ps->shmIdT, IPC_RMID, NULL);
    shmctl(ps->shmIdQ, IPC_RMID, NULL);
}

int assignTerm(PaymentStation *ps, int cid)
{
    for (int i = 0; i < ps->count; i++)
    {
        pthread_mutex_lock(&ps->terms[i].lock);
        if (!ps->terms[i].occupied)
        {
            ps->terms[i].occupied = OCCUPIED;
            ps->terms[i].clientId = cid;
            pthread_mutex_unlock(&ps->terms[i].lock);
            return i;
        }
        pthread_mutex_unlock(&ps->terms[i].lock);
    }
    return -1;
}

void releaseTerm(PaymentStation *ps, int id)
{
    if (id < 0 || id >= ps->count)
        return;
    pthread_mutex_lock(&ps->terms[id].lock);
    ps->terms[id].occupied = FREE;
    ps->terms[id].clientId = -1;
    pthread_mutex_unlock(&ps->terms[id].lock);
}