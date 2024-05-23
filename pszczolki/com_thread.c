#include "main.h"
#include "com_thread.h"

/* wątek komunikacyjny; zajmuje się odbiorem i reakcją na komunikaty */
void *start_com_thread(void *ptr)
{
    MPI_Status status;
    packet_t packet;

    while (state != AFTER_FUNERAL && state!=DEAD ) {
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
        debug("pacet.ts: %d, lamport_clock: %d, priority: %d", packet.ts, lamport_clock, priority);

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
                    changeState(REST);
                }
                else if (state == WAIT_REED || state == WaitForACKReed) {
                    enqueue(reed_queue, status.MPI_SOURCE);
                }
                break;
            case REED_ACK:
                debug("COM THREAD: sended_reed_req_ts: %d, packet.ts %d", sended_reed_req_ts, packet.ts);
                if (packet.reed_id == reed_id && packet.ts >= sended_reed_req_ts && (state == WAIT_REED || state == WaitForACKReed)){
                    pthread_mutex_lock(&ack_reed_count_mut);
                    ack_reed_count++;
                    pthread_mutex_unlock(&ack_reed_count_mut);

                    debug("Dostałem ACK od %d, mam już %d, potrzebuje %d", status.MPI_SOURCE, ack_reed_count, bees-1);
                }
                if(state==WAIT_REED || state==WaitForACKReed){
                    if ( ack_reed_count >= bees - 1){ 
                        changeState(ON_REED);
                    } 
                }
                break;
            case FLOWER_REQUEST:
                debug("Dostałem FLOWER_REQUEST, from: %d, moj state: %s, priority: %d", status.MPI_SOURCE, state_names[state], priority);
                if (state!=WAIT_FLOWER && flower_occupied < NUM_FLOWERS){
                    sendPacket(0, status.MPI_SOURCE, FLOWER_ACK);
                }
                else if (state == WAIT_FLOWER && !priority){
                    sendPacket(0, status.MPI_SOURCE, FLOWER_ACK);
                    changeAckFlowerCount(0);
                    changeState(ON_REED);
                } else if (state == WAIT_FLOWER && priority){
                    pthread_mutex_lock(&queue_flower_mutex);
                    enqueue(flower_queue, status.MPI_SOURCE); // status.MPI_SOURCE
                    pthread_mutex_unlock(&queue_flower_mutex);
                }
                break;
            case FLOWER_ACK:
                // debug("sended_flower_req_ts: %d, packet.ts %d, state: %s", sended_flower_req_ts, packet.ts, state_names[state]);
                if (packet.ts >= sended_flower_req_ts && (state == WAIT_FLOWER)) {
                    pthread_mutex_lock(&ack_flower_count_mut);
                    ack_flower_count++;
                    pthread_mutex_unlock(&ack_flower_count_mut);
                    debug("Dostałem Flower_ACK od %d, mam już %d, potrzebuje %d", status.MPI_SOURCE, ack_flower_count, bees - (NUM_FLOWERS + flower_occupied));
                }
                println("Dostałem FLOWER_ACK, bees: %d, flower_occupied: %d, NUM_FLOWERS: %d, ack count: %d", bees, flower_occupied, NUM_FLOWERS, ack_flower_count);
                if (ack_flower_count >= bees - (NUM_FLOWERS - flower_occupied) && (state == WAIT_FLOWER) && flower_occupied < NUM_FLOWERS){
                    changeState(ON_FLOWER);
                }
                debug("state: %s", state_names[state])
                break;
            case ENTER_FLOWER:
                addOccupiedFlowerCount();
                // flower_occupied++;
                println("Otrzymałem ENTER_FLOWER od pszczółki nr %d, flowers: %d", status.MPI_SOURCE, flower_occupied);
                break;
            case END_FLW:
                subtractOccupiedFlowerCount();
                println("Otrzymałem END_FLOWER od pszczółki nr %d, flowers: %d", status.MPI_SOURCE, flower_occupied);
                if(ack_flower_count >= bees - (NUM_FLOWERS - flower_occupied) && (state == WAIT_FLOWER) && flower_occupied < NUM_FLOWERS) {
                    changeState(ON_FLOWER);
                }
                break;
            case COOCON:
                debug("Otrzymałem COCOON od pszczółki nr %d składa jajo na trzcinie nr %d", status.MPI_SOURCE, packet.reed_id);
                pthread_mutex_lock(&reed_egg_counter_mutex);
                reed_egg_counter[packet.reed_id]++; // zwiększam licznik jajek na danej trzcinie
                pthread_mutex_unlock(&reed_egg_counter_mutex);
                break;
            case END_OF_LIFE:
                bees--;
                debug("Otrzymałem DEAD od pszczółki nr %d, ilosc pszczolek: %d, moj stan: %s", status.MPI_SOURCE, bees,state_names[state]);
                break;
            default:
                break;
        }
    }
    return NULL;
}
