#ifndef QUEUE_H
#define QUEUE_H

#include "simulation.h"
#include <stdbool.h>

// Bağlı liste düğümü
typedef struct QueueNode {
    Vehicle* vehicle;
    struct QueueNode* next;
} QueueNode;

// Kuyruk yapısı
typedef struct {
    QueueNode* front;
    QueueNode* rear;
    int size;
} Queue;

// Global kuyruklar (0: A Yakası, 1: B Yakası)
extern Queue side_queues[2];

// Fonksiyon prototipleri
void init_queue(Queue* q);
void enqueue(Queue* q, Vehicle* v);
Vehicle* dequeue(Queue* q);
bool is_empty(Queue* q);
Vehicle* peek(Queue* q);

#endif