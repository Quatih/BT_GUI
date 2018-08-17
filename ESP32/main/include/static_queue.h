#ifndef _QUEUE_H
#define _QUEUE_H
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
//#define malloc(params) pvPortMalloc

#define QueueType int
#define MAX_SIZE 10
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
    if(queueIsEmpty(q)){
        return (QueueType)0;
    }
    QueueType ret = q->queue[0];
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
        q->size = MAX_SIZE;
    }
    //printf("Value %d added at %d\n", data, q->size-1);
    return true;
}

#endif