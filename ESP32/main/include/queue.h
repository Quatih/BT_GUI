/* 
Queue datastructure implemented with a singly linked list.
*/

#ifndef _QUEUE_H
#define _QUEUE_H
#include <stdlib.h>
#include <stdbool.h>

//#define malloc(params) pvPortMalloc

#define QueueType int
typedef struct s_QueueNode {
    QueueType data;
    struct s_QueueNode *next;
} QueueNode;

typedef struct s_Queue {
    size_t size;
    size_t queue_max;
    QueueNode *start;
    QueueNode *end;
} Queue;

void queueInit(Queue* queue, size_t max_size){
    queue->size = 0;
	queue->start = NULL;
	queue->end = NULL;
    queue->queue_max = max_size;
}

// Create a queue with at most max_size elements, or unrestricted if max_size is 0
Queue *queueCreate(size_t max_size) {
	Queue *queue = malloc(sizeof (Queue));
	if(queue == NULL)
		return NULL;
	
	queue->size = 0;
	queue->start = NULL;
	queue->end = NULL;
    queue->queue_max = max_size;
	return queue;
}

bool queueIsEmpty(Queue * queue){
    return queue->size == 0 ? true : false;
}
bool queueIsFull(Queue * queue){
    return queue->size == queue->queue_max ? true : false;
}

QueueType dequeue (Queue * queue){
    if(queueIsEmpty(queue)){
        return (QueueType) 0;
    }
    else if(queue->size == 1){
        queue->end = NULL;
    }
    QueueNode * oldStart = queue->start;
    QueueType ret = oldStart->data;
    QueueNode * newStart = oldStart->next;

    free(oldStart);
    queue->start = newStart;

    queue->size -= 1;
    return ret;
}

bool enqueue (Queue * queue, QueueType data){
    if (queue == NULL){
        printf("Null queue input\n");
        return false;
    }
    printf("Free heap: %d\n", xPortGetFreeHeapSize());
    QueueNode *node = malloc(sizeof(QueueNode));
    if (node == NULL) {
        printf("Node failed to be created\n");
        return false;
    }
    // else{
    //     printf("Malloc success!\n");
    // }
    node->data = data;
    node->next = NULL;
    // remove the top element if at max size 
    // if queue_max = 0, then unrestricted
    printf("Before: Queue size: %d, Max: %u, Qptr %u, Nptr %u\n", queue->size, queue->queue_max, queue, node);
    printf("null: %d", queue == NULL);
    if (queueIsFull(queue)){
        printf("Before:2 " );
        QueueNode * oldStart = queue->start;
        printf("start: %u, ", oldStart);
        QueueNode * newStart = oldStart->next;
        queue->start = newStart;
        printf("Free, ");
        free(oldStart);
        
        printf("Queue full!, %u\n", queue->size);
    }
    else if(queueIsEmpty(queue)){
        printf("Empty\n");
        queue->start = node;
        queue->size += 1;
    } 
    else {
        printf("Free space\n");
        queue->size += 1;
        queue->end->next = node;
    }
    queue->end = node;
    
    return true;
}


void queueClean(Queue * queue){
    while(!queueIsEmpty(queue)){
        dequeue (queue);
    }
}

void queueFree(Queue * queue){
    queueClean(queue);
    free(queue);
}

#endif