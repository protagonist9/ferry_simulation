#ifndef SIMULATION_H
#define SIMULATION_H

#include <pthread.h>
#include <stdbool.h>

#define SIDE_A 0
#define SIDE_B 1

#define CAR_SIZE 1
#define MINIBUS_SIZE 2
#define TRUCK_SIZE 3

#define NUM_CARS 12
#define NUM_MINIBUSES 10
#define NUM_TRUCKS 8
#define TOTAL_VEHICLES (NUM_CARS + NUM_MINIBUSES + NUM_TRUCKS)

#define FERRY_CAPACITY 20

typedef enum { CAR, MINIBUS, TRUCK } VehicleType;

typedef struct {
    int id;
    VehicleType type;
    int size;
    int origin_side;
    int current_side;
    bool has_completed_trip;
} Vehicle;

typedef struct {
    int current_load;
    int ferry_location;
    bool is_loading;
    bool is_unloading;
    int completed_vehicles;

    pthread_mutex_t ferry_mutex;
    pthread_mutex_t stats_mutex;
    pthread_mutex_t queue_mutex[2];
    pthread_mutex_t toll_mutex[2][2];

    pthread_cond_t cond_ferry_available[2];
    pthread_cond_t cond_ferry_depart;
    pthread_cond_t cond_unloading_done;
} SystemState;

extern SystemState state;

#endif