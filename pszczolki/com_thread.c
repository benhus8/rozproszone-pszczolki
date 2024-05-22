#include "main.h"
#include "com_thread.h"

/* wątek komunikacyjny; zajmuje się odbiorem i reakcją na komunikaty */
void *start_com_thread(void *ptr)
{
    MPI_Status status;
    packet_t packet;

    while (state != AFTER_FUNERAL ) {
        MPI_Recv( &packet, 1, MPI_PACKET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        pthread_mutex_lock( &clock_mut );
        lamport_clock = (lamport_clock > packet.ts ? lamport_clock : packet.ts) + 1;
        pthread_mutex_unlock( &clock_mut );


        // int src_priority = packet.priority;
        int priority = 0;
        if(packet.ts > lamport_clock){
            priority = 1;
        }
        else if(packet.ts == lamport_clock){
            priority = status.MPI_SOURCE > rank ? 1 : 0;
        }
        else{
            priority = 0;
        }
        println("pacet.ts: %d, lamport_clock: %d, priority: %d", packet.ts, lamport_clock, priority);

        switch ( status.MPI_TAG ) {
            case REED_REQUEST:
                debug("O trzcine nr %d ubiega się %d", packet.reed_id, packet.src);
                if (packet.reed_id != reed_id) {
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
            println("COM THREAD: sended_reed_req_ts: %d, packet.ts %d", sended_reed_req_ts, packet.ts);
                if (packet.reed_id == reed_id && packet.ts >= sended_reed_req_ts && (state == WAIT_REED || state == WaitForACKReed)){
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
                println("Otrzymałem COCOON od pszczółki nr %d składa jajo na trzcinie nr %d", status.MPI_SOURCE, packet.reed_id);
                pthread_mutex_lock(&reed_egg_counter_mutex);
                reed_egg_counter[packet.reed_id]++; // zwiększam licznik jajek na danej trzcinie
                pthread_mutex_unlock(&reed_egg_counter_mutex);
                break;
            case END_OF_LIFE:
                println("Otrzymałem DEAD od pszczółki nr %d", status.MPI_SOURCE);
                break;
            default:
                break;
        }
    }
    return NULL;
}
