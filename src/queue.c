#include "../include/queue.h"
#include <stdlib.h>
#include <stddef.h>

Queue side_queues[2];

void init_queue(Queue* q) {
    q->front = NULL;
    q->rear = NULL;
    q->size = 0;
}

void enqueue(Queue* q, Vehicle* v) {
    QueueNode* newNode = (QueueNode*)malloc(sizeof(QueueNode));
    newNode->vehicle = v;
    newNode->next = NULL;

    if (q->rear == NULL) {
        q->front = newNode;
        q->rear = newNode;
    } else {
        q->rear->next = newNode;
        q->rear = newNode;
    }
    q->size++;
}

Vehicle* dequeue(Queue* q) {
    if (q->front == NULL) {
        return NULL;
    }

    QueueNode* temp = q->front;
    Vehicle* v = temp->vehicle;
    
    q->front = q->front->next;
    if (q->front == NULL) {
        q->rear = NULL;
    }
    
    free(temp);
    q->size--;
    return v;
}

bool is_empty(Queue* q) {
    return q->size == 0;
}

Vehicle* peek(Queue* q) {
    if (q->front == NULL) {
        return NULL;
    }
    return q->front->vehicle;
}