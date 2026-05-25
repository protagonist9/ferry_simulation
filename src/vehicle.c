#include "../include/vehicle.h"
#include "../include/simulation.h"
#include "../include/queue.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "../include/logger.h"

void* vehicle_thread(void* arg) {
    Vehicle* v = (Vehicle*)arg;
    // Thread-safe random seed
    unsigned int seed = (unsigned int)(time(NULL) ^ pthread_self() ^ v->id);
    
    for (int trip = 0; trip < 2; trip++) { 
        
        int toll_id = rand_r(&seed) % 2; 
        pthread_mutex_lock(&state.toll_mutex[v->current_side][toll_id]);
        usleep((rand_r(&seed) % 50 + 10) * 1000); 
        log_vehicle_event(v, v->current_side == SIDE_A ? "entered toll on Side A" : "entered toll on Side B");
        pthread_mutex_unlock(&state.toll_mutex[v->current_side][toll_id]);

        pthread_mutex_lock(&state.queue_mutex[v->current_side]);
        enqueue(&side_queues[v->current_side], v);
        log_vehicle_event(v, v->current_side == SIDE_A ? "joined queue on Side A" : "joined queue on Side B");
        long wait_start = get_current_time_ms(); 
        pthread_mutex_unlock(&state.queue_mutex[v->current_side]);

        pthread_mutex_lock(&state.ferry_mutex);
        if (state.ferry_location != v->current_side && state.is_loading && state.current_load == 0) {
            pthread_cond_signal(&state.cond_ferry_depart);
        }
        pthread_mutex_unlock(&state.ferry_mutex);

        pthread_mutex_lock(&state.ferry_mutex);

        while (1) {
            pthread_mutex_lock(&state.queue_mutex[v->current_side]);
            Vehicle* front_vehicle = peek(&side_queues[v->current_side]);
            pthread_mutex_unlock(&state.queue_mutex[v->current_side]);

            if (state.ferry_location == v->current_side && 
                state.is_loading && 
                front_vehicle == v && 
                state.current_load + v->size <= FERRY_CAPACITY) {
                break; 
            }

            if (state.ferry_location == v->current_side && state.is_loading && 
                front_vehicle == v && 
                state.current_load + v->size > FERRY_CAPACITY) {
                pthread_cond_signal(&state.cond_ferry_depart);
            }

            pthread_cond_wait(&state.cond_ferry_available[v->current_side], &state.ferry_mutex);
        }

        pthread_mutex_lock(&state.queue_mutex[v->current_side]);
        dequeue(&side_queues[v->current_side]);
        pthread_mutex_unlock(&state.queue_mutex[v->current_side]);
        
        state.current_load += v->size;
        
        long wait_time = get_current_time_ms() - wait_start;
        record_wait_time(wait_time);
        log_vehicle_event(v, "boarded the ferry");

        pthread_cond_broadcast(&state.cond_ferry_available[v->current_side]);

        if (state.current_load == FERRY_CAPACITY) {
            pthread_cond_signal(&state.cond_ferry_depart);
        }

        while (state.ferry_location == v->current_side || !state.is_unloading) {
            pthread_cond_wait(&state.cond_unloading_done, &state.ferry_mutex);
        }

        state.current_load -= v->size;
        v->current_side = 1 - v->current_side; 
        log_vehicle_event(v, "unloaded from the ferry");
        
        if (state.current_load == 0) {
            pthread_cond_signal(&state.cond_ferry_depart);
        }

        pthread_mutex_unlock(&state.ferry_mutex);

        if (trip == 0) {
            usleep((rand_r(&seed) % 500 + 100) * 1000); 
        }
    }

    v->has_completed_trip = true;
    
    pthread_mutex_lock(&state.stats_mutex);
    state.completed_vehicles++;
    pthread_mutex_unlock(&state.stats_mutex);

    // Bitiş gecikmesi çözümü: Feribota tüm araçların bittiğini hemen bildir
    pthread_mutex_lock(&state.ferry_mutex);
    pthread_cond_broadcast(&state.cond_ferry_depart);
    pthread_mutex_unlock(&state.ferry_mutex);

    return NULL;
}