#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include "../include/simulation.h"
#include "../include/queue.h"
#include "../include/vehicle.h"
#include "../include/ferry.h"
#include "../include/logger.h"

SystemState state;

void init_system() {
    state.current_load = 0;
    state.ferry_location = rand() % 2; 
    state.is_loading = false;
    state.is_unloading = false;
    state.completed_vehicles = 0;

    pthread_mutex_init(&state.ferry_mutex, NULL);
    pthread_mutex_init(&state.stats_mutex, NULL);
    
    for (int side = 0; side < 2; side++) {
        pthread_mutex_init(&state.queue_mutex[side], NULL);
        init_queue(&side_queues[side]);
        pthread_cond_init(&state.cond_ferry_available[side], NULL);
        
        for (int toll = 0; toll < 2; toll++) {
            pthread_mutex_init(&state.toll_mutex[side][toll], NULL);
        }
    }
    
    pthread_cond_init(&state.cond_ferry_depart, NULL);
    pthread_cond_init(&state.cond_unloading_done, NULL);
}

void clean_system() {
    pthread_mutex_destroy(&state.ferry_mutex);
    pthread_mutex_destroy(&state.stats_mutex);
    
    for (int side = 0; side < 2; side++) {
        pthread_mutex_destroy(&state.queue_mutex[side]);
        pthread_cond_destroy(&state.cond_ferry_available[side]);
        
        for (int toll = 0; toll < 2; toll++) {
            pthread_mutex_destroy(&state.toll_mutex[side][toll]);
        }
    }
    
    pthread_cond_destroy(&state.cond_ferry_depart);
    pthread_cond_destroy(&state.cond_unloading_done);
}

int main() {
    srand(time(NULL));
    
    init_system();
    init_logger();
    
    pthread_t ferry_tid;
    pthread_t vehicle_tids[TOTAL_VEHICLES];
    Vehicle vehicles[TOTAL_VEHICLES];

    printf("[SİSTEM] Feribot simülasyonu başlatılıyor...\n");

    if (pthread_create(&ferry_tid, NULL, ferry_thread, NULL) != 0) {
        perror("Feribot thread'i oluşturulamadı");
        return 1;
    }

    int v_idx = 0;
    
    for (int i = 0; i < NUM_CARS; i++, v_idx++) {
        vehicles[v_idx].id = i + 1;
        vehicles[v_idx].type = CAR;
        vehicles[v_idx].size = CAR_SIZE;
        vehicles[v_idx].origin_side = rand() % 2;
        vehicles[v_idx].current_side = vehicles[v_idx].origin_side;
        vehicles[v_idx].has_completed_trip = false;
        pthread_create(&vehicle_tids[v_idx], NULL, vehicle_thread, &vehicles[v_idx]);
    }
    
    for (int i = 0; i < NUM_MINIBUSES; i++, v_idx++) {
        vehicles[v_idx].id = i + 1;
        vehicles[v_idx].type = MINIBUS;
        vehicles[v_idx].size = MINIBUS_SIZE;
        vehicles[v_idx].origin_side = rand() % 2;
        vehicles[v_idx].current_side = vehicles[v_idx].origin_side;
        vehicles[v_idx].has_completed_trip = false;
        pthread_create(&vehicle_tids[v_idx], NULL, vehicle_thread, &vehicles[v_idx]);
    }
    
    for (int i = 0; i < NUM_TRUCKS; i++, v_idx++) {
        vehicles[v_idx].id = i + 1;
        vehicles[v_idx].type = TRUCK;
        vehicles[v_idx].size = TRUCK_SIZE;
        vehicles[v_idx].origin_side = rand() % 2;
        vehicles[v_idx].current_side = vehicles[v_idx].origin_side;
        vehicles[v_idx].has_completed_trip = false;
        pthread_create(&vehicle_tids[v_idx], NULL, vehicle_thread, &vehicles[v_idx]);
    }

    for (int i = 0; i < TOTAL_VEHICLES; i++) {
        pthread_join(vehicle_tids[i], NULL);
    }

    pthread_join(ferry_tid, NULL);

    printf("[SİSTEM] Tüm araçlar turlarını tamamladı. Simülasyon güvenli bir şekilde kapatılıyor.\n");
    
    clean_system();
    print_statistics();
    
    return 0;
}