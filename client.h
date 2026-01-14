#ifndef CLIENT_H
#define CLIENT_H

#include "pump.h"
#include "payment.h"
#include "management.h"

typedef struct
{
    int id;
    int fuelType;
    int liters;
    PumpStation *pumps;
    PaymentStation *payments;
    Stats *stats;
} ClientInfo;

void *clientThread(void *arg);

#endif