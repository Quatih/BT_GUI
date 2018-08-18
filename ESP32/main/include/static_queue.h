#ifndef _QUEUE_H
#define _QUEUE_H
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>

//#define malloc(params) pvPortMalloc

#define QueueType int
#define QueueTime uint32_t
#define MAX_SIZE 10
typedef struct {
    QueueType data;
    QueueTime time;
} dequeue_tuple;

typedef struct {
    QueueType queue[MAX_SIZE];
    QueueTime timestamp[MAX_SIZE]; 
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

QueueTime get_msecs() {

    struct timeval tv;

    gettimeofday(&tv, NULL);
    QueueTime ret = (QueueTime) (tv.tv_sec * 1000LL + (tv.tv_usec / 1000LL));
    return ret;
}

dequeue_tuple dequeue(Static_Queue* q){
    dequeue_tuple ret = {NULL, NULL};
    if(queueIsEmpty(q)){
        return ret;
    }
    
    ret.data = q->queue[0];
    ret.time = q->timestamp[0];
    for (size_t i = 0; i < q->size ; i++){
        q->queue[i] = q->queue[i+1];
        q->timestamp[i] = q->timestamp[i+1];
    }
    q->size--;
    //printf("deq: %d\n", ret);
    return ret;
}


bool enqueue(Static_Queue* q, QueueType data){
    if( q->size < MAX_SIZE){
        q->queue[q->size] = data;
        q->timestamp[q->size] = get_msecs();
        q->size++;
    }
    else{
        //printf("overload\n");
        dequeue(q);
        q->queue[MAX_SIZE-1] = data;
        q->timestamp[MAX_SIZE-1] = get_msecs();
        q->size = MAX_SIZE;
    }
    //printf("Value %d added at %d\n", data, q->size-1);
    return true;
}

#endif