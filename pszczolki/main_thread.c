#include "main.h"
#include "main_thread.h"

void main_loop()
{
    srandom(rank);

    while (state != DEAD) {
		
		packet_t* pkt = malloc(sizeof(packet_t));
		
		switch (state) {			
			case REST: 
			changeState(WAIT_REED);
				// wait
				break;
			case WAIT_REED:
				debug("Czekam na trzcine")
				println("Czekam na trzcinę")
				
				pkt->reed_id = reed_id;
				pkt->priority = rec_priority;

				for (int i = 0; i < size-1; i++){
					if (i != rank){
						sendPacket(pkt, i, REED_REQUEST);
					}
				} changeState(WaitForACKReed);
				free(pkt);
				break;
			case WaitForACKReed:
				// wait
				break;
			case ON_REED:
				debug("Jestem na trzcinie")
				println("Jestem na trzcinie")
				rec_priority = lamport_clock*1000 + rank;
				pkt->priority = rec_priority;

				pkt->reed_id = reed_id;
				for (int i = 0; i <= size-1; i++){
					if (i != rank){
						sendPacket(pkt, i, FLOWER_REQUEST);
					}
				} changeState(WAIT_FLOWER);
				free(pkt);
				break;
			case WAIT_FLOWER:
				// wait
				break;
			case ON_FLOWER:
				println("Jestem na kwiatku %d !!!!!!!!", reed_id);

				sleep(10); 

				println("Skończyłem byc na kwiatku %d !!!!!!!!", reed_id);

				debug("Wychodzę z kwiatka")
				debug("Zmieniam stan na wysyłanie");

				pthread_mutex_lock(&queue_flower_mutex);
				while (!isEmpty(flower_queue)){
					int dest = dequeue(flower_queue);
					sendPacket(0, dest, FLOWER_ACK);
				}
				pthread_mutex_unlock(&queue_flower_mutex);
				changeAckReedCount(0);
				changeAckFlowerCount(0);
				changeState( EGG);
				break;
			case EGG:
				println("Składam jajko")
				sleep(10);
				egg_count++;
				for (int i = 0; i <= size-1; i++){
					if (i != rank){
						sendPacket(pkt, i, COOCON);
					}
				} 
				free(pkt);
				changeState(ON_REED);
				break;
			case DEAD:
				break;
			default: 
				break;
			}
		sleep(SEC_IN_STATE);
	}
}
