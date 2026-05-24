# Ferry Transport Simulation

This project is a multithreaded ferry system simulation transporting vehicles between two sides (Side A and Side B), developed using C and POSIX threads (pthreads). The system aims to solve concurrency, mutual exclusion, and synchronization problems in operating systems.

## Technical Details and Synchronization

The system architecture is strictly based on the sleep/wakeup principle, explicitly avoiding any "busy waiting" loops.

* **Thread Management:** Each of the 30 vehicles (Car, Minibus, Truck) and the Ferry operate asynchronously as independent `pthread` instances.
* **Mutual Exclusion (Mutex):** Toll booth passages, FIFO queue enqueue/dequeue operations, and ferry load capacity updates are protected by `pthread_mutex` locks to prevent race conditions.
* **State Synchronization (Condition Variables):** Vehicles waiting for the ferry, the ferry waiting for capacity, and boarding/unloading permissions are coordinated using `pthread_cond_wait` and `pthread_cond_signal` / `broadcast` mechanisms.
* **Deadlock and Starvation Prevention:** The lock acquisition hierarchy is strictly maintained to eliminate deadlock risks. A maximum waiting time (timeout) is implemented for ferry departures to ensure cross-side coordination and prevent vehicle starvation.

## Project Structure

The modular file hierarchy for the project is as follows:

    .
    ├── Makefile
    ├── include/
    │   ├── ferry.h          # Ferry state machine prototypes
    │   ├── logger.h         # Statistics and logging prototypes
    │   ├── queue.h          # Thread-safe FIFO queue prototypes
    │   ├── simulation.h     # Global locks, condition variables, and state structs
    │   └── vehicle.h        # Vehicle thread prototypes
    └── src/
        ├── ferry.c          # Ferry loading/travel/unloading loops
        ├── logger.c         # Time-stamped I/O operations and metric calculations
        ├── main.c           # Thread initialization and resource (lock) management
        ├── queue.c          # Linked-list based queue implementation
        └── vehicle.c        # Vehicle toll, boarding, unloading, and return logic

## Requirements

The project relies on standard POSIX libraries. A Linux environment, the `gcc` compiler, and the `make` utility are required for compilation.

## How to Build and Run

Navigate to the project root directory (where the `Makefile` is located) and execute the following command in your terminal to compile and start the simulation:

    make && ./ferry_sim

Once the simulation starts, all toll passages, queue waiting times, and ferry departure/arrival events are printed to the console with millisecond timestamps.

The simulation safely self-terminates when all vehicles complete their round trips (60 trips total). It returns all acquired locks and allocated memory to the operating system and prints a final statistics report containing total simulation time, ferry capacity utilization ratio, and average waiting times.

To clean up compiled object files and the executable, run:

    make clean