#include "main.h"
#include "main_thread.h"
#include "com_thread.h"
#include "queue.h"
#include "util.h"

/*
 * W main.h extern int rank (zapowiedź) w main.c int rank (definicja)
 * Zwróćcie uwagę, że każdy proces ma osobą pamięć, ale w ramach jednego
 * procesu wątki współdzielą zmienne - więc dostęp do nich powinien
 * być obwarowany muteksami. Rank i size akurat są write-once, więc nie trzeba,
 * ale zob util.c oraz util.h - zmienną state_t state i funkcję changeState
 *
 */
struct Queue* reed_queue = NULL;
struct Queue* flower_queue = NULL;
int reed_id = 0;
pthread_t com_thread;
int reed_egg_counter[NUM_REEDS] = {0};
int sended_reed_req_ts = 0;
int sended_flower_req_ts = 0;

void finalize()
{
    pthread_mutex_destroy( &state_mut);
    /* Czekamy, aż wątek potomny się zakończy */
    println("czekam na wątek \"komunikacyjny\"\n" );
    pthread_join(com_thread,NULL);
    MPI_Type_free(&MPI_PACKET_T);
    MPI_Finalize();
}

void check_thread_support(int provided)
{
    // printf("THREAD SUPPORT: chcemy %d. Co otrzymamy?\n", provided);
    switch (provided) {
        case MPI_THREAD_SINGLE: 
            printf("Brak wsparcia dla wątków, kończę\n");
            /* Nie ma co, trzeba wychodzić */
	    fprintf(stderr, "Brak wystarczającego wsparcia dla wątków - wychodzę!\n");
	    MPI_Finalize();
	    exit(-1);
	    break;
        case MPI_THREAD_FUNNELED: 
            printf("tylko te wątki, ktore wykonaly mpi_init_thread mogą wykonać wołania do biblioteki mpi\n");
	    break;
        case MPI_THREAD_SERIALIZED: 
            /* Potrzebne zamki wokół wywołań biblioteki MPI */
            printf("tylko jeden watek naraz może wykonać wołania do biblioteki MPI\n");
	    break;
        case MPI_THREAD_MULTIPLE: // printf("Pełne wsparcie dla wątków\n"); /* tego chcemy. Wszystkie inne powodują problemy */
	    break;
        default: printf("Nikt nic nie wie\n");
    }
}

int main(int argc, char **argv)
{
    reed_queue = createQueue();
    flower_queue = createQueue();
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    check_thread_support(provided);
    srand(rank);
    /* zob. util.c oraz util.h */
    init_packet_type(); // tworzy typ pakietu
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    // komunikacja (odbieranie i wysyłanie REQ, ACK...)
    pthread_create( &com_thread, NULL, start_com_thread , 0);
    // główny wątek (zmiana stanu w którym jest krasnolud na danym etapie)
    main_loop();

    finalize();
    return 0;
}
