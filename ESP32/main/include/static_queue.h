#ifndef _QUEUE_H
#define _QUEUE_H
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
//#define malloc(params) pvPortMalloc

#define QueueType int
#define MAX_SIZE 20
typedef struct {
    QueueType queue[MAX_SIZE];
    size_t size;
} Static_Queue;

void queueClear(Static_Queue* q){
    for(size_t i = MAX_SIZE; i != 0; i--){
        q->queue[i] = 0;
    }
    q->size = 0;
}

Static_Queue *queueCreate() {
    //printf("Allocating %u\n", sizeof(Static_Queue));
	Static_Queue *queue = (Static_Queue*) malloc(sizeof (Static_Queue));
	if(queue == NULL)
		return NULL;
	
	queueClear(queue);
	return queue;
}

bool queueIsEmpty(Static_Queue* q){
    return q->size == 0 ? true : false;
}

void queueOverWrite(Static_Queue* q, QueueType data){
    q->queue[q->size] = data;
}

QueueType dequeue(Static_Queue* q){
    QueueType ret = q->queue[0];
    for (size_t i = q->size-1; i > 0 ; i--){
        q->queue[i-1] = q->queue[i];
    }
    q->size--;
    return ret;
}

bool enqueue(Static_Queue* q, QueueType data){
    if( q->size < (MAX_SIZE - 1)){
        q->queue[q->size] = data;
        q->size++;
    }
    else{
        //printf("overload\n");
        for (size_t i = q->size-1; i > 0 ; i--){
            q->queue[i-1] = q->queue[i];
        }
        q->queue[MAX_SIZE-1] = data;
    }
    return true;
}

#endif