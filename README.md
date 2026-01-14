# PumpStation

**PumpStation** is a C-based simulation system designed to model the operations of a gas station. Developed as a university project, the program manages the interaction between clients, fuel pumps, payment processing, and central management, simulating a real-world service environment.

## Features

* **Station Management:** Centralized control to oversee station operations and resources.
* **Pump Simulation:** Independent management of fuel pumps, handling states (idle, fueling, stopped).
* **Client Interaction:** Simulation of client arrival, fuel requests, and payment processes.
* **Payment System:** Logic to handle transaction processing.
* **Queue Management:** Implementation of FIFO queues to manage client wait times or request processing order.

## Operations

**Client Operations:**

* **Request Fuel:** Client arrives and requests a specific pump or fuel type.
* **Make Payment:** Process payment for the fueled amount.
* **Queue:** Wait in line if all pumps are busy.

**Station Operations:**

* **Authorize Pump:** The management system authorizes a pump to start fueling after validation.
* **Monitor Status:** Track the status of all pumps (Free, Busy, Out of Service).
* **Process Transaction:** Calculate costs and update revenue records.

**System Mechanics:**

* **FIFO Implementation:** Uses a First-In-First-Out data structure to handle service order fairly.
* **State Machines:** Pumps operate on a state machine model (Waiting -> Active -> Payment -> Done).

## Technologies

* C Programming Language
* Standard Input/Output
* FIFO Data Structures (Queues)
* Modular Programming (Separation of concerns: Payment, Pump, Client, Management)

## Installation

**Clone the repository:**
```bash
git clone [https://github.com/joaomartinscode/PumpStation.git](https://github.com/joaomartinscode/PumpStation.git)
