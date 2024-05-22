#ifndef MAINH
#define MAINH
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include "util.h"
#include "queue.h"

/* tutaj TYLKO zapowiedzi - definicje w main.c */
extern int rank;
extern int size;
extern int reed_id;
extern int egg_count;
extern int flower_occupied;
extern int lamport_clock;
extern int rec_priority; // priorytet ustalany przy dostaniu zlecenia
extern int reed_egg_counter[NUM_REEDS]; //licznik ile złożono jajek na trzcinie

extern int sended_reed_req_ts;


extern pthread_t com_thread;
extern struct Queue* reed_queue; 
extern struct Queue* flower_queue; 

#endif
