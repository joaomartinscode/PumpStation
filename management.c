#include "management.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>

void initManager(ManagerArgs *args, PumpStation *pumps, PaymentStation *payments)
{
    args->pumps = pumps;
    args->payments = payments;
    args->running = 1;

    args->stats.totalClients = 0;
    args->stats.revenue = 0.0;
    args->stats.totalPumpWaitTime = 0;
    args->stats.totalPaymentWaitTime = 0;

    for (int i = 0; i < 3; i++)
        args->stats.liters[i] = 0;

    pthread_mutex_init(&args->stats.lock, NULL);
}

void destroyManager(ManagerArgs *args)
{
    pthread_mutex_destroy(&args->stats.lock);
}

void *managerThread(void *arg)
{
    ManagerArgs *m = (ManagerArgs *)arg;
    int qid = msgget(QUEUE_KEY, 0666 | IPC_CREAT);

    if (qid == -1)
    {
        perror("manager msgget");
        return NULL;
    }

    printf("[Manager] Started\n");
    ClientToManagerMsg msg;
    ManagerToClientMsg resp;

    while (m->running)
    {
        msgrcv(qid, &msg, sizeof(ClientToManagerMsg) - sizeof(long), SERVER_TYPE, 0);

        switch (msg.action)
        {
        case ACT_REQ_PUMP:
        {
            int pid = assignPump(m->pumps, msg.clientId);
            if (pid == -1)
            {
                int minQ = 0;
                int minSize = fifoSize(&m->pumps->queue[0]);

                for (int i = 1; i < m->pumps->count; i++)
                {
                    int currSize = fifoSize(&m->pumps->queue[i]);
                    if (currSize < minSize)
                    {
                        minSize = currSize;
                        minQ = i;
                    }
                }
                fifoPush(&m->pumps->queue[minQ], msg.clientId);
                printf("Client %d queued at pump %d (size of line = %d)\n", msg.clientId, minQ, minSize + 1);
            }
            else
            {
                memset(&resp, 0, sizeof(ManagerToClientMsg));
                resp.mtype = msg.clientId;
                resp.resourceId = pid;
                msgsnd(qid, &resp, sizeof(ManagerToClientMsg) - sizeof(long), 0);
                printf("Assigned Pump %d to Client %d\n", pid, msg.clientId);
            }
            break;
        }
        case ACT_END_FUEL:
        {
            int pid = msg.resourceId;
            releasePump(m->pumps, pid);
            printf("Client released Pump %d\n", pid);

            if (!fifoIsEmpty(&m->pumps->queue[pid]))
            {
                int next = fifoPop(&m->pumps->queue[pid]);

                pthread_mutex_lock(&m->pumps->pumps[pid].lock);
                m->pumps->pumps[pid].occupied = OCCUPIED;
                m->pumps->pumps[pid].clientId = next;
                pthread_mutex_unlock(&m->pumps->pumps[pid].lock);

                memset(&resp, 0, sizeof(ManagerToClientMsg));
                resp.mtype = next;
                resp.resourceId = pid;
                msgsnd(qid, &resp, sizeof(ManagerToClientMsg) - sizeof(long), 0);
                printf("Assigned Pump %d to Client %d (from queue)\n", pid, next);
            }
            break;
        }
        case ACT_REQ_PAY:
        {
            int tid = assignTerm(m->payments, msg.clientId);
            if (tid == -1)
            {
                int minQ = 0;
                int minSize = fifoSize(&m->payments->queue[0]);

                for (int i = 1; i < m->payments->count; i++)
                {
                    int currSize = fifoSize(&m->payments->queue[i]);
                    if (currSize < minSize)
                    {
                        minSize = currSize;
                        minQ = i;
                    }
                }

                fifoPush(&m->payments->queue[minQ], msg.clientId);
                printf("Client %d queued at terminal %d (size=%d)\n",
                       msg.clientId, minQ, minSize + 1);
            }
            else
            {
                memset(&resp, 0, sizeof(ManagerToClientMsg));
                resp.mtype = msg.clientId;
                resp.resourceId = tid;
                msgsnd(qid, &resp, sizeof(ManagerToClientMsg) - sizeof(long), 0);
                printf("Assigned Terminal %d to Client %d\n", tid, msg.clientId);
            }
            break;
        }
        case ACT_END_PAY:
        {
            int tid = msg.resourceId;
            releaseTerm(m->payments, tid);
            printf("Client released Terminal % d\n ", tid);

            if (!fifoIsEmpty(&m->payments->queue[tid]))
            {
                int next = fifoPop(&m->payments->queue[tid]);

                pthread_mutex_lock(&m->payments->terms[tid].lock);
                m->payments->terms[tid].occupied = OCCUPIED;
                m->payments->terms[tid].clientId = next;
                pthread_mutex_unlock(&m->payments->terms[tid].lock);

                memset(&resp, 0, sizeof(ManagerToClientMsg));
                resp.mtype = next;
                resp.resourceId = tid;
                msgsnd(qid, &resp, sizeof(ManagerToClientMsg) - sizeof(long), 0);
                printf("Assigned Terminal %d to Client %d (from queue)\n", tid, next);
            }
            pthread_mutex_lock(&m->stats.lock);
            m->stats.totalClients++;
            pthread_mutex_unlock(&m->stats.lock);
            break;
        }
        case ACT_EXIT:
            m->running = 0;
            printf("Exit signal received.\n");
            break;
        }
    }
    printf("\nDETAILED REPORT:\n\n");

    printf("PUMP STATISTICS:\n");
    for (int i = 0; i < m->pumps->count; i++)
    {
        printf("  Pump %d:\n", i);
        printf("    Fuel 95:     %d liters\n", m->pumps->pumps[i].liters[0]);
        printf("    Fuel 98:     %d liters\n", m->pumps->pumps[i].liters[1]);
        printf("    Fuel Diesel: %d liters\n", m->pumps->pumps[i].liters[2]);
        int total = m->pumps->pumps[i].liters[0] + m->pumps->pumps[i].liters[1] + m->pumps->pumps[i].liters[2];
        printf("    Total:       %d liters\n\n", total);
    }

    printf("TERMINAL STATISTICS:\n");
    for (int i = 0; i < m->payments->count; i++)
    {
        printf("  Terminal %d:\n", i);
        printf("    Payments: %d\n", m->payments->terms[i].totalPayments);
        printf("    Revenue:  %.2f EUR\n\n", m->payments->terms[i].revenue);
    }

    int totalLiters[3] = {0, 0, 0};
    float totalRevenue = 0.0;
    int totalPayments = 0;

    for (int i = 0; i < m->pumps->count; i++)
    {
        totalLiters[0] += m->pumps->pumps[i].liters[0];
        totalLiters[1] += m->pumps->pumps[i].liters[1];
        totalLiters[2] += m->pumps->pumps[i].liters[2];
    }

    for (int i = 0; i < m->payments->count; i++)
    {
        totalRevenue += m->payments->terms[i].revenue;
        totalPayments += m->payments->terms[i].totalPayments;
    }

    printf("GLOBAL STATISTICS:\n");
    printf("  Total Clients Served: %d\n", totalPayments);
    printf("  Total Fuel 95:        %d liters\n", totalLiters[0]);
    printf("  Total Fuel 98:        %d liters\n", totalLiters[1]);
    printf("  Total Fuel Diesel:    %d liters\n", totalLiters[2]);
    printf("  Total Fuel:           %d liters\n", totalLiters[0] + totalLiters[1] + totalLiters[2]);
    printf("  Total Revenue:        %.2f EUR\n", totalRevenue);

    if (totalPayments > 0)
    {
        printf("  Avg Wait Time (Pump):    %.2f seconds\n",
               (float)m->stats.totalPumpWaitTime / (float)totalPayments);
        printf("  Avg Wait Time (Payment): %.2f seconds\n",
               (float)m->stats.totalPaymentWaitTime / (float)totalPayments);
    }

    printf("\n[Manager] Shutting down.\n");
    msgctl(qid, IPC_RMID, NULL);
    return NULL;
}

float getFuelPrice(int fuelType)
{
    switch (fuelType)
    {
    case 0:
        return FUEL_PRICE_95;
        break;
    case 1:
        return FUEL_PRICE_98;
        break;
    case 2:
        return FUEL_PRICE_DIESEL;
        break;
    default:
        return 0.0;
    }
}
