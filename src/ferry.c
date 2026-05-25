#include "../include/ferry.h"
#include "../include/simulation.h"
#include "../include/queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include "../include/logger.h"

#define FERRY_MAX_WAIT_MS 3000 

void* ferry_thread(void* arg) {
    // Unused parameter (kullanılmayan parametre) uyarısını susturur
    (void)arg; 
    
    // Thread-safe random seed
    unsigned int seed = (unsigned int)(time(NULL) ^ pthread_self());

    while (1) {
        pthread_mutex_lock(&state.stats_mutex);
        if (state.completed_vehicles == TOTAL_VEHICLES) {
            pthread_mutex_unlock(&state.stats_mutex);
            break; 
        }
        pthread_mutex_unlock(&state.stats_mutex);

        pthread_mutex_lock(&state.ferry_mutex);
        state.is_unloading = false;
        state.is_loading = true;

        pthread_cond_broadcast(&state.cond_ferry_available[state.ferry_location]);

        pthread_mutex_lock(&state.queue_mutex[state.ferry_location]);
        int current_side_count = side_queues[state.ferry_location].size;
        pthread_mutex_unlock(&state.queue_mutex[state.ferry_location]);

        pthread_mutex_lock(&state.queue_mutex[1 - state.ferry_location]);
        int opposite_side_count = side_queues[1 - state.ferry_location].size;
        pthread_mutex_unlock(&state.queue_mutex[1 - state.ferry_location]);

        if (current_side_count == 0 && opposite_side_count > 0 && state.current_load == 0) {
            // Pas geç, hemen kalk
        } else {
            // Hassas timeout hesaplaması
            struct timeval now;
            struct timespec timeout;
            gettimeofday(&now, NULL);
            
            long wait_ms = FERRY_MAX_WAIT_MS;
            long nsec = now.tv_usec * 1000 + (wait_ms % 1000) * 1000000;
            timeout.tv_sec = now.tv_sec + (wait_ms / 1000) + (nsec / 1000000000);
            timeout.tv_nsec = nsec % 1000000000;

            pthread_cond_timedwait(&state.cond_ferry_depart, &state.ferry_mutex, &timeout);
        }

        pthread_mutex_lock(&state.stats_mutex);
        if (state.completed_vehicles == TOTAL_VEHICLES) {
            pthread_mutex_unlock(&state.stats_mutex);
            pthread_mutex_unlock(&state.ferry_mutex);
            break; 
        }
        pthread_mutex_unlock(&state.stats_mutex);

        state.is_loading = false;
        record_ferry_trip(state.current_load);
        log_ferry_event(state.ferry_location == SIDE_A ? "departed from Side A" : "departed from Side B", state.current_load);
        
        pthread_mutex_unlock(&state.ferry_mutex);

        // Seyahat
        usleep((rand_r(&seed) % 200 + 300) * 1000); 

        pthread_mutex_lock(&state.ferry_mutex);
        
        state.ferry_location = 1 - state.ferry_location;
        log_ferry_event(state.ferry_location == SIDE_A ? "arrived at Side A" : "arrived at Side B", state.current_load);

        state.is_unloading = true;
        pthread_cond_broadcast(&state.cond_unloading_done);

        while (state.current_load > 0) {
            pthread_cond_wait(&state.cond_ferry_depart, &state.ferry_mutex); 
        }
        
        pthread_mutex_unlock(&state.ferry_mutex);
    }

    return NULL;
}