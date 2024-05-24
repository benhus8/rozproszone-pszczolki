#include "main.h"
#include "main_thread.h"

void main_loop()
{
    // srandom(rank);

	rec_priority = lamport_clock;

    while (state != AFTER_FUNERAL) {
		packet_t* pkt = malloc(sizeof(packet_t));
		switch (state) {			
			case REST: 
				rec_priority = lamport_clock;
				changeState(WAIT_REED);
				// wait
				break;
			case WAIT_REED:
				rec_priority = lamport_clock;
				srand(rank+1);
				// losowo dobieramy trzcinę do której chcemy się dostać 
				reed_id = rand() % NUM_REEDS;
				println("Chcę na trzcinę: %d", reed_id);

				//sprawdzamy czy trzcina na którą chcemy się dostaćnie jest przepłniona
				if (reed_egg_counter[reed_id] >= MAX_REED_COCOON){
					println("Trzcina %d jest przepłniona, szukam innej", reed_id);
					changeState(REST);
					break;
				}

				pkt->priority = rec_priority;
				pkt->reed_id = reed_id;

				changeAckReedCount(0);
				// zapisujemy tiemstampa wysłania requesta
				sended_reed_req_ts = lamport_clock;
				for (int i = 0; i < size; i++){
					if (i != rank){
						sendPacket(pkt, i, REED_REQUEST);
					}
				} 
				
				changeState(WaitForACKReed);
				break;
			case WaitForACKReed:
				rec_priority = lamport_clock;
				if (reed_egg_counter[reed_id] >= MAX_REED_COCOON){
					println("Trzcina %d jest przepłniona, szukam innej", reed_id);
					changeState(REST);
					break;
				}
				else if (bees==1){
					debug("Jestem sam, wchodze bo moge na trzcine");
					changeState(ON_REED);
				}
				break;
			case ON_REED:
				rec_priority = lamport_clock;
				println("Jestem na trzcinie %d", reed_id);

				changeAckFlowerCount(0);

				pkt->reed_id = reed_id;
				pkt->priority = rec_priority;
				sended_flower_req_ts = rec_priority;
				for (int i = 0; i < size; i++){
					if (i != rank){
						sendPacket(pkt, i, FLOWER_REQUEST);
					}
				}

				changeState(WAIT_FLOWER);
			
				break;
			case WAIT_FLOWER:
				if(bees==1){
					changeState(ON_FLOWER);
					debug("Jestem sam, wchodze bo moge na kwiatka");
				}
				break;
			case ON_FLOWER:
				debug("Rozsyłam ENTER_FLOWER do wszystkich ilość zajętych kwiatków wynosi: %d ", flower_occupied);
				rec_priority = lamport_clock;
				pkt->priority = rec_priority;
				for (int i = 0; i < size; i++){
					if (i != rank){
						sendPacket(pkt, i, ENTER_FLOWER);
					}
				}
				addOccupiedFlowerCount();

				println("Jestem na kwiatku, moja trzcina - %d , zajetych kwiatkow: %d", reed_id, flower_occupied);

				sleep(1); 

				println("Skończyłem byc na kwiatku, wracam na trzcine - %d !!!!!!!!", reed_id);

				debug("Wychodzę z kwiatka");

				rec_priority = lamport_clock;
				pkt->priority = rec_priority;
				for (int i = 0; i < size; i++){
					if (i != rank){
						sendPacket(pkt, i, END_FLW);
					}
				}
				subtractOccupiedFlowerCount();

				// printQueue(flower_queue);
				pthread_mutex_lock(&queue_flower_mutex);
				while (!isEmpty(flower_queue)){
					int dest = dequeue(flower_queue);
					sendPacket(0, dest, FLOWER_ACK);
				}
				pthread_mutex_unlock(&queue_flower_mutex);
				
				//zmieniamy stan na zkładanie jajka
				changeState(EGG);
				break;
			case EGG:
				println("Składam jajko numer: %d", egg_count+1);
				sleep(1);
				egg_count++;
				rec_priority = lamport_clock;
				pkt->reed_id = reed_id;
				pkt->priority = rec_priority;
				for (int i = 0; i <= size-1; i++){
					if (i != rank){
						sendPacket(pkt, i, COOCON);
					}
				} 
				if(egg_count >= MAX_EGG){
					println("Zdechłem, złożyłem już %d jajek", egg_count);
					changeState(DEAD);
					break;
				}
				else{
					changeState(ON_REED);
				}
				break;
			case DEAD:
				println("Zdechłem dzownie do wszystkich na pogrzeb i zwalniam kolejkę");
				rec_priority = lamport_clock;
				pkt->priority = rec_priority;
				pkt->reed_id = reed_id;

				for (int i = 0; i < size; i++){
					if (i != rank){
						sendPacket(pkt, i, END_OF_LIFE);
					}
				}

				// printQueue(reed_queue);
				pthread_mutex_lock(&queue_reed_mutex);
				while(!isEmpty(reed_queue)){
					int dest = dequeue(reed_queue);
					sendPacket(0, dest, REED_ACK);
				}
				pthread_mutex_unlock(&queue_reed_mutex);
				

				changeState(AFTER_FUNERAL);
				free(pkt);
				break;
			default: 
				break;
			}
		sleep(SEC_IN_STATE);
	}
}
