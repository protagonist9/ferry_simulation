#include "../include/logger.h"
#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>

static struct timeval start_time;
static long total_wait_time = 0;
static long max_wait_time = 0;
static int total_ferry_trips = 0;
static int total_ferry_load = 0;
static int total_wait_count = 0;

void init_logger() {
    gettimeofday(&start_time, NULL);
}

long get_current_time_ms() {
    struct timeval now;
    gettimeofday(&now, NULL);
    return (now.tv_sec - start_time.tv_sec) * 1000 + (now.tv_usec - start_time.tv_usec) / 1000;
}

const char* get_type_name(VehicleType type) {
    switch(type) {
        case CAR: return "Car";
        case MINIBUS: return "Minibus";
        case TRUCK: return "Truck";
        default: return "Unknown";
    }
}

void log_vehicle_event(Vehicle* v, const char* action) {
    pthread_mutex_lock(&state.stats_mutex);
    printf("[Time %ld] %s-%d %s\n", get_current_time_ms(), get_type_name(v->type), v->id, action);
    pthread_mutex_unlock(&state.stats_mutex);
}

void log_ferry_event(const char* action, int current_load) {
    pthread_mutex_lock(&state.stats_mutex);
    printf("[Time %ld] Ferry %s (current load = %d)\n", get_current_time_ms(), action, current_load);
    pthread_mutex_unlock(&state.stats_mutex);
}

void record_wait_time(long wait_ms) {
    pthread_mutex_lock(&state.stats_mutex);
    total_wait_time += wait_ms;
    total_wait_count++;
    if (wait_ms > max_wait_time) {
        max_wait_time = wait_ms;
    }
    pthread_mutex_unlock(&state.stats_mutex);
}

void record_ferry_trip(int load) {
    pthread_mutex_lock(&state.stats_mutex);
    total_ferry_trips++;
    total_ferry_load += load;
    pthread_mutex_unlock(&state.stats_mutex);
}

void print_statistics() {
    long total_sim_time = get_current_time_ms();
    double avg_wait = total_wait_count > 0 ? (double)total_wait_time / total_wait_count : 0.0;
    
    // Hata veren satır düzeltildi (etiket kaldırıldı)
    double utilization = total_ferry_trips > 0 ? ((double)total_ferry_load / (total_ferry_trips * FERRY_CAPACITY)) * 100.0 : 0.0;

    printf("\n========================================\n");
    printf("         SIMULATION STATISTICS\n");
    printf("========================================\n");
    printf("Total simulation time   : %ld ms\n", total_sim_time);
    printf("Average waiting time    : %.2f ms\n", avg_wait);
    printf("Maximum waiting time    : %ld ms\n", max_wait_time);
    printf("Number of ferry trips   : %d\n", total_ferry_trips);
    printf("Ferry utilization ratio : %.2f%%\n", utilization);
    printf("========================================\n");
}