#include "main.h"
#include "com_thread.h"

/* wątek komunikacyjny; zajmuje się odbiorem i reakcją na komunikaty */
void *start_com_thread(void *ptr)
{
    MPI_Status status;
    packet_t packet;

    while (state != DEAD ) {
        MPI_Recv( &packet, 1, MPI_PACKET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        pthread_mutex_lock( &clock_mut );
        lamport_clock = (lamport_clock > packet.ts ? lamport_clock : packet.ts) + 1;
        pthread_mutex_unlock( &clock_mut );

        int src_priority = packet.priority;
        int priority = rec_priority < src_priority ? 1 : 0; // mamy priorytet, jeśli mamy mniejszą wartość

        switch ( status.MPI_TAG ) {
            case REED_REQUEST: 
                debug("Priorytety: rec %ld, src %d", rec_priority, src_priority);
                debug("O trzcine ubiega się %d", packet.src);
                if (packet.reed_id != reed_id){
                    packet_t *pkt = malloc(sizeof(packet_t));
                    pkt->reed_id = packet.reed_id;
                    sendPacket( pkt, status.MPI_SOURCE, REED_ACK );
                }
                else if ((state == WAIT_REED || state == WaitForACKReed) && !priority){
                    debug("Rezygnuję z trzciny, %d ma wyższy priorytet", packet.src);
                    packet_t *pkt = malloc(sizeof(packet_t));
                    pkt->reed_id = reed_id;
                    sendPacket( pkt, status.MPI_SOURCE, REED_ACK );
                    changeAckReedCount(0);
                    reed_queue = createQueue();
                    changeState(REST);
                }
                else if (state == WAIT_REED || state == WaitForACKReed) {
                    enqueue(reed_queue, packet.src);
                }
                break;
            case REED_ACK:
                if (packet.reed_id == reed_id){
                    pthread_mutex_lock(&ack_reed_count_mut);
                    ack_reed_count++;
                    pthread_mutex_unlock(&ack_reed_count_mut);

                    debug("Dostałem ACK od %d, mam już %d, potrzebuje %d", status.MPI_SOURCE, ack_reed_count, NUM_BEES-1); /* czy potrzeba tutaj muteksa? Będzie wyścig, czy nie będzie? Zastanówcie się. */
                    if ( ack_reed_count >= NUM_BEES - 1){ 
                        changeState(ON_REED);
                    } 
                }
                break;
            case FLOWER_REQUEST:
                debug("Dostałem flower request");
                if (state != ON_FLOWER && state != WAIT_FLOWER){
                    sendPacket(0, status.MPI_SOURCE, FLOWER_ACK);
                }
                else if (state != REST && !priority){
                    sendPacket(0, status.MPI_SOURCE, FLOWER_ACK);
                } else if ((state != REST && priority) || state == ON_FLOWER ){
                    pthread_mutex_lock(&queue_flower_mutex);
                    enqueue(flower_queue, packet.src); // status.MPI_SOURCE
                    pthread_mutex_unlock(&queue_flower_mutex);
                }
                break;
            case FLOWER_ACK:
                if (ack_flower_count >= NUM_BEES - NUM_FLOWERS + flower_occupied){
                    changeState(ON_FLOWER);
                }
                else {
                    pthread_mutex_lock(&ack_flower_count_mut);
                    ack_flower_count++;
                    pthread_mutex_unlock(&ack_flower_count_mut);
                    debug("Dostałem Flower_ACK od %d, mam już %d, potrzebuje %d", status.MPI_SOURCE, ack_flower_count, NUM_BEES - 1 - NUM_FLOWERS + flower_occupied);
                    if (ack_flower_count >= NUM_BEES - NUM_FLOWERS + flower_occupied){
                        changeState(ON_FLOWER);
                    }
                }
                break;
            case COOCON:
                break;
            default:
                break;
        }
    }
    return NULL;
}
