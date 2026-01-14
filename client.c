#include "client.h"
#include "management.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>

static int getRandom(int min, int max)
{
    return min + rand() % (max - min + 1);
}

void *clientThread(void *arg)
{
    ClientInfo *c = (ClientInfo *)arg;
    int qid = msgget(QUEUE_KEY, 0666 | IPC_CREAT);

    if (qid == -1)
    {
        perror("client msgget");
        free(c);
        return NULL;
    }

    printf("Client %d arrived (Fuel: %d, Liters: %d)\n", c->id, c->fuelType, c->liters);

    ClientToManagerMsg req;
    ManagerToClientMsg resp;

    time_t pumpRequestTime = time(NULL);

    memset(&req, 0, sizeof(ClientToManagerMsg));
    req.mtype = SERVER_TYPE;
    req.action = ACT_REQ_PUMP;
    req.clientId = c->id;
    req.resourceId = 0;
    msgsnd(qid, &req, sizeof(ClientToManagerMsg) - sizeof(long), 0);

    msgrcv(qid, &resp, sizeof(ManagerToClientMsg) - sizeof(long), c->id, 0);
    time_t pumpAssignedTime = time(NULL);
    int pumpWaitTime = pumpAssignedTime - pumpRequestTime;

    int myPump = resp.resourceId;
    printf("Client %d -> Pump %d (waited %d seconds)\n", c->id, myPump, pumpWaitTime);

    pthread_mutex_lock(&c->stats->lock);
    c->stats->totalPumpWaitTime += pumpWaitTime;
    pthread_mutex_unlock(&c->stats->lock);

    int refuelTime = 1 + (c->liters / 10);
    printf("Client %d refueling %d liters of fuel type %d at Pump %d (will take %d seconds)\n",
           c->id, c->liters, c->fuelType, myPump, refuelTime);
    sleep(refuelTime);

    pthread_mutex_lock(&c->pumps->pumps[myPump].statsLock);
    c->pumps->pumps[myPump].liters[c->fuelType] += c->liters;
    pthread_mutex_unlock(&c->pumps->pumps[myPump].statsLock);

    memset(&req, 0, sizeof(ClientToManagerMsg));
    req.mtype = SERVER_TYPE;
    req.action = ACT_END_FUEL;
    req.clientId = c->id;
    req.resourceId = myPump;
    msgsnd(qid, &req, sizeof(ClientToManagerMsg) - sizeof(long), 0);

    time_t paymentRequestTime = time(NULL);

    memset(&req, 0, sizeof(ClientToManagerMsg));
    req.mtype = SERVER_TYPE;
    req.action = ACT_REQ_PAY;
    req.clientId = c->id;
    req.resourceId = 0;
    msgsnd(qid, &req, sizeof(ClientToManagerMsg) - sizeof(long), 0);

    msgrcv(qid, &resp, sizeof(ManagerToClientMsg) - sizeof(long), c->id, 0);
    time_t paymentAssignedTime = time(NULL);
    int paymentWaitTime = paymentAssignedTime - paymentRequestTime;

    int myTerm = resp.resourceId;
    printf("Client %d -> Terminal %d (waited %d seconds)\n", c->id, myTerm, paymentWaitTime);

    pthread_mutex_lock(&c->stats->lock);
    c->stats->totalPaymentWaitTime += paymentWaitTime;
    pthread_mutex_unlock(&c->stats->lock);

    int paymentTime = 1 + (getRandom(0, 1));
    printf("Client %d paying %.2f EUR at Terminal %d (will take %d seconds)\n",
           c->id, c->liters * getFuelPrice(c->fuelType), myTerm, paymentTime);
    sleep(paymentTime);

    pthread_mutex_lock(&c->payments->terms[myTerm].statsLock);
    c->payments->terms[myTerm].totalPayments++;
    c->payments->terms[myTerm].revenue += c->liters * getFuelPrice(c->fuelType);
    pthread_mutex_unlock(&c->payments->terms[myTerm].statsLock);

    memset(&req, 0, sizeof(ClientToManagerMsg));
    req.mtype = SERVER_TYPE;
    req.action = ACT_END_PAY;
    req.clientId = c->id;
    req.resourceId = myTerm;
    msgsnd(qid, &req, sizeof(ClientToManagerMsg) - sizeof(long), 0);

    printf("Client %d left.\n", c->id);
    free(c);
    return NULL;
}