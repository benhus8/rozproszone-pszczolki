#include "util.h"

int rank, size;
int ack_reed_count, ack_flower_count = 0;
int egg_count = 0;
int flower_occupied = 0;
int lamport_clock = 0;
int rec_priority = 0;
MPI_Datatype MPI_PACKET_T;
state_t state = REST;

int bees = NUM_BEES;
/* zamek wokół zmiennej współdzielonej między wątkami. 
 * Zwróćcie uwagę, że każdy proces ma osobą pamięć, ale w ramach jednego
 * procesu wątki współdzielą zmienne - więc dostęp do nich powinien
 * być obwarowany muteksami
 */
pthread_mutex_t state_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t clock_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ack_reed_count_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ack_flower_count_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_reed_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_flower_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t reed_egg_counter_mutex = PTHREAD_MUTEX_INITIALIZER;

struct tagNames_t{
    const char *name;
    int tag;
} tagNames[] = { {"żądanie wejścia do sekcji trzciny", ENTER_REED}, {"żądanie wejścia do sekcji kwiatka", ENTER_FLOWER}, {"żądanie wejścia do sekcji gniazda", COOCON},
                {"żądanie zakończenia sekcji kwiatka", END_FLW}, {"żądanie zakończenia sekcji gniazda", END_OF_LIFE}, {"potwierdzenie wejścia do sekcji trzciny", REED_ACK},
                {"prośba o sekcję krytyczną trzciny", REED_REQUEST}, {"prośba o kwiatek", FLOWER_REQUEST}, {"potwierdzenie odn. kwiatka", FLOWER_ACK}
                };

char *state_names[] = {
    "REST", 
    "WAIT_REED", 
    "ON_REED", 
    "WAIT_FLOWER",
    "ON_FLOWER",
    "WaitForACKReed",
    "WaitForACKFlower",
    "EGG",
    "DEAD",
    "AFTER_FUNERAL"
};

const char *const tag2string( int tag )
{
    for (int i=0; i <sizeof(tagNames)/sizeof(struct tagNames_t);i++) {
	if ( tagNames[i].tag == tag )  return tagNames[i].name;
    }
    return "<unknown>";
}

void init_packet_type()
{
    int blocklengths[NITEMS] = {1,1,1,1};
    MPI_Datatype types[NITEMS] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT};

    MPI_Aint offsets[NITEMS]; 
    offsets[0] = offsetof(packet_t, ts);
    offsets[1] = offsetof(packet_t, src);
    offsets[2] = offsetof(packet_t, reed_id);
    offsets[3] = offsetof(packet_t, priority);
    

    MPI_Type_create_struct(NITEMS, blocklengths, offsets, types, &MPI_PACKET_T);

    MPI_Type_commit(&MPI_PACKET_T);
}

void sendPacket(packet_t *pkt, int destination, int tag)
{
    int freepkt=0;
    if (pkt==0) { pkt = malloc(sizeof(packet_t)); freepkt=1;}

    pkt->src = rank;

    pthread_mutex_lock( &clock_mut );
    lamport_clock++;
    pkt->ts = lamport_clock;
    pthread_mutex_unlock( &clock_mut );

    MPI_Send( pkt, 1, MPI_PACKET_T, destination, tag, MPI_COMM_WORLD);
    debug("Wysyłam %s do %d, moj ts: %d", tag2string(tag), destination, pkt->ts);
    if (freepkt) free(pkt);
}

void changeState( state_t new_state )
{
    pthread_mutex_lock( &state_mut );
    if (state==AFTER_FUNERAL) { 
	    pthread_mutex_unlock( &state_mut );
        return;
    }
    state = new_state;
    pthread_mutex_unlock( &state_mut );
}

void changeAckReedCount(int value){
    pthread_mutex_lock(&ack_reed_count_mut);
    ack_reed_count = value;
    pthread_mutex_unlock(&ack_reed_count_mut);
}

void changeAckFlowerCount(int value){
    pthread_mutex_lock(&ack_flower_count_mut);
    ack_flower_count = value;
    pthread_mutex_unlock(&ack_flower_count_mut);
}