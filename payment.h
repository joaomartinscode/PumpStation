#ifndef PAYMENT_H
#define PAYMENT_H
#include <pthread.h>
#include "fifo.h"

#define FREE 0
#define OCCUPIED 1

typedef struct
{
    int occupied;
    int clientId;
    int totalPayments;
    float revenue;

    pthread_mutex_t lock;
    pthread_mutex_t statsLock;
} Terminal;

typedef struct
{
    Terminal *terms;
    FifoQueue *queue;
    int count;
    int shmIdT;
    int shmIdQ;
} PaymentStation;

void initTerms(PaymentStation *ps, int count);
void destroyTerms(PaymentStation *ps);
int assignTerm(PaymentStation *ps, int cid);
void releaseTerm(PaymentStation *ps, int id);

#endif