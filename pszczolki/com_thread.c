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


        int priority = 0;
        if(packet.priority > rec_priority){
            priority = 1;
        }
        else if(packet.priority == rec_priority){
            priority = status.MPI_SOURCE > rank ? 1 : 0;
        }

        debug("priority: %d, packet priority: %d, rec: %d", priority, packet.priority, rec_priority);  

        switch ( status.MPI_TAG ) {
            case REED_REQUEST:
                if (packet.reed_id != reed_id) {
                    debug("Nie jestem zainteresowany trzciną %d", packet.reed_id);
                    packet_t *pkt = malloc(sizeof(packet_t));
                    pkt->reed_id = packet.reed_id;
                    sendPacket( pkt, status.MPI_SOURCE, REED_ACK );
                }
                else if ((state == WAIT_REED || state == WaitForACKReed) && !priority){
                    debug("Rezygnuję z trzciny, %d ma wyższy priorytet rowny: %d, a moj: %d, finalnie: %d", packet.src, packet.priority, rec_priority, priority);
                    packet_t *pkt = malloc(sizeof(packet_t));
                    pkt->reed_id = reed_id;
                    sendPacket( pkt, status.MPI_SOURCE, REED_ACK );
                }
                else {
                    debug("Mam wyzszy priorytet:), state: %s", state_names[state]);
                    enqueue(reed_queue, status.MPI_SOURCE);
                }
                break;
            case REED_ACK:
                if((state==WAIT_REED || state==WaitForACKReed) && reed_egg_counter[reed_id] < MAX_REED_COCOON){
                    if ( ack_reed_count >= NUM_BEES - 1){ 
                        changeState(ON_REED);
                    } 
                }
                else{
                    println("O nieee trzcina jest przepełniona");
                }

                if (packet.ts > sended_reed_req_ts && (state == WAIT_REED || state == WaitForACKReed)){
                    pthread_mutex_lock(&ack_reed_count_mut);
                    ack_reed_count++;
                    pthread_mutex_unlock(&ack_reed_count_mut);

                    debug("Dostałem ACK od %d, mam już %d, potrzebuje %d", status.MPI_SOURCE, ack_reed_count, bees-1);
                    if ( ack_reed_count >= NUM_BEES - 1){ 
                        changeState(ON_REED);
                    } 
                }
                debug("ack: %d", ack_reed_count);
                break;
            case FLOWER_REQUEST:
                debug("Dostałem FLOWER_REQUEST, from: %d, moj state: %s, priority: %d rec_priority %d, source priority %d, sending %d", status.MPI_SOURCE, state_names[state], priority, rec_priority, packet.priority, sending);
                if(state != AFTER_FUNERAL && state != DEAD) {
                    if(state == ON_FLOWER && egg_count + 1 >= MAX_EGG){
                        pthread_mutex_lock(&queue_flower_mutex);
                        enqueue(flower_queue, status.MPI_SOURCE); 
                        pthread_mutex_unlock(&queue_flower_mutex);
                    }
                    else if(state == EGG && egg_count == MAX_EGG){
                        pthread_mutex_lock(&queue_flower_mutex);
                        enqueue(flower_queue, status.MPI_SOURCE); 
                        pthread_mutex_unlock(&queue_flower_mutex);
                    }
                    else if (state!=WAIT_FLOWER && state!=ON_REED && state!=ON_FLOWER){
                        sendPacket(0, status.MPI_SOURCE, FLOWER_ACK);
                    }
                    else if( state==ON_REED && sending == 0){
                        sendPacket(0, status.MPI_SOURCE, FLOWER_ACK);
                    }
                    else if ((state == WAIT_FLOWER || (state == ON_REED && sending == 1)) && !priority){
                        sendPacket(0, status.MPI_SOURCE, FLOWER_ACK);
                    } else{
                        pthread_mutex_lock(&queue_flower_mutex);
                        enqueue(flower_queue, status.MPI_SOURCE); // status.MPI_SOURCE
                        pthread_mutex_unlock(&queue_flower_mutex);
                        debug("zapamietuje sobie ciebie");
                    }
                }
                break;
            case FLOWER_ACK:
                if((state == WAIT_FLOWER || state == ON_REED) && flower_occupied < NUM_FLOWERS && ack_flower_count >= bees - (NUM_FLOWERS)){
                    changeState(ON_FLOWER);
                }

                if (packet.ts >= sended_flower_req_ts && (state == WAIT_FLOWER || state == ON_REED)) {
                    pthread_mutex_lock(&ack_flower_count_mut);
                    ack_flower_count++;
                    pthread_mutex_unlock(&ack_flower_count_mut);

                    debug("Dostałem Flower_ACK od %d, mam już %d, potrzebuje %d", status.MPI_SOURCE, ack_flower_count, bees - (NUM_FLOWERS));

                    if ( ack_flower_count >= bees - (NUM_FLOWERS)){ 
                        changeState(ON_FLOWER);
                    }
                }
                debug("ack: %d source: %d bees %d", ack_flower_count, status.MPI_SOURCE, bees);
                break;
            case ENTER_FLOWER:
                addOccupiedFlowerCount();
                debug("Otrzymałem ENTER_FLOWER od pszczółki nr %d, flowers: %d"2, status.MPI_SOURCE, flower_occupied);
                break;
            case END_FLW:
                subtractOccupiedFlowerCount();
                debug("Otrzymałem END_FLOWER od pszczółki nr %d, flowers: %d", status.MPI_SOURCE, flower_occupied);
                break;
            case COOCON:
                debug("Otrzymałem COCOON od pszczółki nr %d składa jajo na trzcinie nr %d", status.MPI_SOURCE, packet.reed_id);
                pthread_mutex_lock(&reed_egg_counter_mutex);
                reed_egg_counter[packet.reed_id]++; // zwiększam licznik jajek na danej trzcinie
                pthread_mutex_unlock(&reed_egg_counter_mutex);
                break;
            case END_OF_LIFE:
                bees--;
                if ( (state == ON_REED || state == WAIT_FLOWER ) && ack_flower_count >= bees - (NUM_FLOWERS)){ 
                    changeState(ON_FLOWER);
                }
                debug("Otrzymałem DEAD od pszczółki nr %d, ilosc pszczolek: %d, moj stan: %s", status.MPI_SOURCE, bees,state_names[state]);
                break;
            default:
                break;
        }
    }
    return NULL;
}
