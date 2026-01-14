#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/msg.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "pump.h"
#include "payment.h"
#include "client.h"
#include "management.h"

/*
Instituto Politécnico de Portalegre
Sistemas Operativos
Trabalho prático 3 - PumpStation
Engenharia Informática - 2º Ano

Alunos:
    Joao Martins	no. 25856
    Paulo Campos	no. 25721

Data de Realização: 10/01/2026
*/

int main(int argc, char *argv[])
{

    int nPumps = (argc >= 2) ? atoi(argv[1]) : 4;
    int nTerms = (argc >= 3) ? atoi(argv[2]) : 2;
    int nClients = (argc >= 4) ? atoi(argv[3]) : 10;

    srand(time(NULL));

    PumpStation pumps;
    PaymentStation payments;
    ManagerArgs mgr;

    initPumps(&pumps, nPumps);
    initTerms(&payments, nTerms);
    initManager(&mgr, &pumps, &payments);

    pthread_t managerTid;
    pthread_create(&managerTid, NULL, managerThread, &mgr);

    pthread_t *clients = malloc(sizeof(pthread_t) * nClients);
    for (int i = 0; i < nClients; i++)
    {
        ClientInfo *c = malloc(sizeof(ClientInfo));
        c->id = i + 1;
        c->fuelType = rand() % 3;
        c->liters = 20 + rand() % 31;
        c->pumps = &pumps;
        c->payments = &payments;
        c->stats = &mgr.stats;

        pthread_create(&clients[i], NULL, clientThread, c);
        if (i < nClients - 1)
            sleep(rand() % 3);
    }

    for (int i = 0; i < nClients; i++)
    {
        pthread_join(clients[i], NULL);
    }
    printf(">>> All clients finished. Stopping manager...\n");

    int qid = msgget(QUEUE_KEY, 0666);
    ClientToManagerMsg exitMsg;
    exitMsg.mtype = SERVER_TYPE;
    exitMsg.action = ACT_EXIT;
    exitMsg.clientId = 0;
    exitMsg.resourceId = 0;
    msgsnd(qid, &exitMsg, sizeof(ClientToManagerMsg) - sizeof(long), 0);

    pthread_join(managerTid, NULL);

    destroyManager(&mgr);
    destroyPumps(&pumps);
    destroyTerms(&payments);
    free(clients);

    printf("Simulation Complete.\n");
    return 0;
}