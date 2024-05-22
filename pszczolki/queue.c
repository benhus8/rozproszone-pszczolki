#include "queue.h"
#include <stdio.h>
#include <stdlib.h>

struct QueueNode* createQueueNode(int value) {
    struct QueueNode* newNode = (struct QueueNode*)malloc(sizeof(struct QueueNode));
    newNode->data = value;
    newNode->next = NULL;
    return newNode;
}

struct Queue* createQueue() {
    struct Queue* queue = (struct Queue*)malloc(sizeof(struct Queue));
    queue->front = NULL;
    queue->rear = NULL;
    queue->size = 0;
    return queue;
}

int isEmpty(struct Queue* queue) {
    return (queue->front == NULL);
}

void enqueue(struct Queue* queue, int value) {
    struct QueueNode* newNode = createQueueNode(value);
    if (isEmpty(queue)) {
        queue->front = newNode;
        queue->rear = newNode;
    } else {
        queue->rear->next = newNode;
        queue->rear = newNode;
    }
    queue->size++;
}

int dequeue(struct Queue* queue) {
    if (isEmpty(queue)) {
        printf("The queue is empty.\n");
        return -1;
    }
    struct QueueNode* temp = queue->front;
    int dequeuedValue = temp->data;

    if (queue->front == queue->rear) {
        queue->front = NULL;
        queue->rear = NULL;
    } else {
        queue->front = queue->front->next;
    }

    free(temp);
    queue->size--;
    return dequeuedValue;
}

void printQueue(struct Queue* queue) {
    if (isEmpty(queue)) {
        printf("The queue is empty.\n");
        return;
    }

    struct QueueNode* current = queue->front;
    while (current != NULL) {
        printf("%d ", current->data);
        current = current->next;
    }
    printf("\n");
}

void deleteNode(struct Queue* queue, int value) {
    struct QueueNode* current = queue->front;
    struct QueueNode* previous = NULL;

    while (current != NULL && current->data != value) {
        previous = current;
        current = current->next;
    }

    if (current != NULL) {
        if (previous == NULL) {
            queue->front = current->next;
        }
        else if (current == queue->rear) {
            queue->rear = previous;
            previous->next = NULL;
        }
        else {
            previous->next = current->next;
        }
        free(current);
        queue->size--;
    }
}