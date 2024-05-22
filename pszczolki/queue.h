#ifndef QUEUEH
#define QUEUEH

struct QueueNode {
    int data;
    struct QueueNode* next;
};

struct Queue {
    struct QueueNode* front;
    struct QueueNode* rear;
    int size;
};

struct Queue* createQueue();
int isEmpty(struct Queue* queue);
void enqueue(struct Queue* queue, int value);
int dequeue(struct Queue* queue);
void printQueue(struct Queue* queue);
void deleteNode(struct Queue* queue, int value);

#endif
