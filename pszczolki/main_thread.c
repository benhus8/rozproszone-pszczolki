#include "main.h"
#include "main_thread.h"

void main_loop()
{
    srandom(rank);

    while (state != AFTER_FUNERAL) {
		packet_t* pkt = malloc(sizeof(packet_t));
		switch (state) {			
			case REST: 
			changeState(WAIT_REED);
				// wait
				break;
			case WAIT_REED:
				println("Czekam na trzcinę");
				// losowo dobieramy trzcinę do której chcemy się dostać 
				reed_id = rand() % NUM_REEDS;

				//sprawdzamy czy trzcina na którą chcemy się dostaćnie jest przepłniona
				if (reed_egg_counter[reed_id] >= MAX_REED_COCOON){
					println("Trzcina %d jest przepłniona, szukam innej", reed_id);
					changeState(REST);
					break;
				}

				println("Chcę na trzcinę: %d", reed_id);
				pkt->reed_id = reed_id;
				pkt->priority = rec_priority;

				// zapisujemy tiemstampa wysłania requesta
				sended_reed_req_ts = lamport_clock;
				for (int i = 0; i < size; i++){
					if (i != rank){
						sendPacket(pkt, i, REED_REQUEST);
					}
				} 
				
				changeState(WaitForACKReed);
				free(pkt);
				break;
			case WaitForACKReed:
				if (reed_egg_counter[reed_id] >= MAX_REED_COCOON){
					println("Trzcina %d jest przepłniona, szukam innej", reed_id);
					changeState(REST);
					break;
				}
				break;
			case ON_REED:
				debug("Jestem na trzcinie %d", reed_id);
				println("Jestem na trzcinie %d", reed_id);
				rec_priority = lamport_clock*1000 + rank;
				pkt->priority = rec_priority;

				pkt->reed_id = reed_id;
				for (int i = 0; i < size; i++){
					if (i != rank){
						sendPacket(pkt, i, FLOWER_REQUEST);
					}
				}

				pthread_mutex_lock(&queue_reed_mutex);
				while (!isEmpty(reed_queue)){
					int dest = dequeue(reed_queue);
					sendPacket(0, dest, REED_ACK);
				}
				pthread_mutex_unlock(&queue_reed_mutex);
				changeAckReedCount(0);

				if(egg_count >= MAX_EGG){
					println("Zdechłem, złożyłem już %d jajek", egg_count);
					changeState(DEAD);
					break;
				}
				else{
					changeState(WAIT_FLOWER);
				}
			
				free(pkt);
				break;
			case WAIT_FLOWER:
				// wait
				break;
			case ON_FLOWER:
				println("Jestem na kwiatku, moja trzcina - %d ", reed_id);

				sleep(1); 

				println("Skończyłem byc na kwiatku, wracam na trzcine - %d !!!!!!!!", reed_id);

				debug("Wychodzę z kwiatka");
				debug("Zmieniam stan na wysyłanie");

				pthread_mutex_lock(&queue_flower_mutex);
				while (!isEmpty(flower_queue)){
					int dest = dequeue(flower_queue);
					sendPacket(0, dest, FLOWER_ACK);
				}
				pthread_mutex_unlock(&queue_flower_mutex);
				changeAckFlowerCount(0);

				//zmieniamy stan na zkładanie jajka
				changeState(EGG);
				break;
			case EGG:
				println("Składam jajko numer: %d", egg_count+1);
				sleep(1);
				egg_count++;
				pkt->reed_id = reed_id;
				for (int i = 0; i <= size-1; i++){
					if (i != rank){
						sendPacket(pkt, i, COOCON);
					}
				} 
				free(pkt);
				changeState(ON_REED);
				break;
			case DEAD:
				println("Zdechłem dzownie do wszystkich na pogrzeb");
				for (int i = 0; i < size; i++){
					if (i != rank){
						sendPacket(pkt, i, END_OF_LIFE);
					}
				}
				changeState(AFTER_FUNERAL);
				free(pkt);
				break;
			default: 
				break;
			}
		sleep(SEC_IN_STATE);
	}
}
