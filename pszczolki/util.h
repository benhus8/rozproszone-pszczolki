#ifndef UTILH
#define UTILH
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <pthread.h>
#include <stddef.h>

#ifdef DEBUG
#define debug(FORMAT,...) printf("%c[%d;%dm [%d] [%d]: " FORMAT "%c[%d;%dm\n",  27, (1+(rank/7))%2, 31+(6+rank)%7, rank, lamport_clock, ##__VA_ARGS__, 27,0,37);
#else
#define debug(...) ;
#endif

// makro println - to samo co debug, ale wyświetla się zawsze
#define println(FORMAT,...) printf("%c[%d;%dm [%d] [%d]: " FORMAT "%c[%d;%dm\n",  27, (1+(rank/7))%2, 31+(6+rank)%7,rank, lamport_clock, ##__VA_ARGS__, 27,0,37);

#ifndef NUM_REEDS
    #define NUM_REEDS 2
#endif

#ifndef NUM_FLOWERS
    #define NUM_FLOWERS 2
#endif

#ifndef NUM_BEES
    #define NUM_BEES 3
#endif

#define TRUE 1
#define FALSE 0
#define SEC_IN_STATE 1
#define STATE_CHANGE_PROB 10

/* packet_t ma 4 pola, więc NITEMS=4. Wykorzystane w inicjuj_typ_pakietu */
#define NITEMS 4

#define MAX_EGG 5
#define MAX_REED_COCOON 15

/* TYPY PAKIETÓW */
#define REED_ACK     1
#define REED_REQUEST 2
#define FLOWER_ACK 3
#define FLOWER_REQUEST 4
#define ENTER_REED 5
#define ENTER_FLOWER 6
#define COOCON 7
#define END_FLW 8
#define END_OF_LIFE 9

typedef struct {
    int ts;      
    int src;  
    int reed_id;
    int priority;
} packet_t;

typedef enum {
    REST, 
    WAIT_REED, 
    ON_REED, 
    WAIT_FLOWER,
    ON_FLOWER,
    WaitForACKReed,
    WaitForACKFlower,
    EGG,
    DEAD,
    AFTER_FUNERAL
} state_t;

extern int rank;
extern int size;
extern int egg_count;
extern int flower_occupied;
extern int ack_reed_count;
extern int ack_flower_count;
extern int lamport_clock;
extern int rec_priority;

extern MPI_Datatype MPI_PACKET_T;
extern state_t state;
extern pthread_mutex_t state_mut;
extern pthread_mutex_t clock_mut;
extern pthread_mutex_t ack_reed_count_mut;
extern pthread_mutex_t ack_flower_count_mut;
extern pthread_mutex_t queue_reed_mutex;
extern pthread_mutex_t queue_flower_mutex;

extern pthread_mutex_t reed_egg_counter_mutex;

void init_packet_type();
void sendPacket(packet_t *pkt, int destination, int tag);
void changeState(state_t);
void changeAckReedCount(int);
void changeAckFlowerCount(int);

#endif


