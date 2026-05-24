#ifndef LOGGER_H
#define LOGGER_H

#include "simulation.h"

void init_logger();
long get_current_time_ms();
void log_vehicle_event(Vehicle* v, const char* action);
void log_ferry_event(const char* action, int current_load);
void record_wait_time(long wait_ms);
void record_ferry_trip(int load);
void print_statistics();

#endif