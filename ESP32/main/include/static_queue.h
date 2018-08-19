#ifndef _QUEUE_H
#define _QUEUE_H
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>

typedef struct {
    int data;
    uint32_t time;
} val_tuple;

#define QueueType val_tuple
#define QueueTime uint32_t
#define MAX_SIZE 1000

typedef struct {
    QueueType queue[MAX_SIZE];
    size_t size;
} Static_Queue;

Static_Queue *queueCreate() {
    printf("Allocating %u\n", sizeof(Static_Queue));
	Static_Queue *queue = (Static_Queue*) malloc(sizeof (Static_Queue));
	if(queue == NULL)
		return NULL;

    queue->size = 0;
	return queue;
}

bool queueIsEmpty(Static_Queue* q){
    return q->size == 0 ? true : false;
}

void queueOverWrite(Static_Queue* q, QueueType data){
    q->queue[q->size] = data;
}

QueueType dequeue(Static_Queue* q){
    QueueType ret;
    if(queueIsEmpty(q)){
        return ret;
    }
    ret = q->queue[0];

    for (size_t i = 0; i < q->size ; i++){
        q->queue[i] = q->queue[i+1];
    }
    q->size--;
    //printf("deq: %d\n", ret);
    return ret;
}


bool enqueue(Static_Queue* q, QueueType data){
    if( q->size < MAX_SIZE){
        q->queue[q->size] = data;
        q->size++;
    }
    else{
        //printf("overload\n");
        dequeue(q);
        q->queue[MAX_SIZE-1] = data;
        q->size = MAX_SIZE;
    }
    //printf("Value %d added at %d\n", data, q->size-1);
    return true;
}

#endif