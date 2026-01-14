#ifndef MANAGEMENT_H
#define MANAGEMENT_H

#include <pthread.h>
#include "pump.h"
#include "payment.h"

#define QUEUE_KEY 12345
#define SERVER_TYPE 999

#define FUEL_PRICE_95 1.50
#define FUEL_PRICE_98 1.65
#define FUEL_PRICE_DIESEL 1.40

#define ACT_REQ_PUMP 1
#define ACT_END_FUEL 2
#define ACT_REQ_PAY 3
#define ACT_END_PAY 4
#define ACT_EXIT 99

typedef struct
{
    long mtype;
    int action;
    int clientId;
    int resourceId;
} ClientToManagerMsg;

typedef struct
{
    long mtype;
    int resourceId;
} ManagerToClientMsg;

typedef struct
{
    int totalClients;
    int liters[3];
    double revenue;
    int totalPumpWaitTime;
    int totalPaymentWaitTime;
    pthread_mutex_t lock;
} Stats;

typedef struct
{
    PumpStation *pumps;
    PaymentStation *payments;
    Stats stats;
    int running;
} ManagerArgs;

void initManager(ManagerArgs *args, PumpStation *pumps, PaymentStation *payments);
void destroyManager(ManagerArgs *args);
void *managerThread(void *arg);
float getFuelPrice(int fuelType);

#endif