#include <mpi.h> //with openmpi use #include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

pthread_mutex_t mutex;
pthread_mutex_t mutexClock;
sem_t semaphore;

typedef struct Packet_t {
    int clock_d;
    int queueNumber;
    int direction;
    int tid;
    int requestType;
} Packet;

int clock_d = 0;
int myTid;
int world_size;

void* send_loop(void *id){
    while(1){
        printf("Hello from func:  processor %d, out of %d processors\n",
               myTid, world_size);
        sleep(5);
    }
}

int main(int argc, char** argv) {
    // Initialize the MPI environment
    MPI_Init(&argc, &argv);

    // Get the number of processes
    MPI_Comm_rank (MPI_COMM_WORLD, &myTid);        /* get current process id */
    MPI_Comm_size (MPI_COMM_WORLD, &world_size);

    pthread_attr_t attr;
    pthread_attr_init(&attr );
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_mutex_init (&mutex, nullptr);
    pthread_mutex_init (&mutexClock, nullptr);
    sem_init(&semaphore,1,1);
    sem_wait(&semaphore);

    pthread_t threadId;
    pthread_create(&threadId, &attr, send_loop, nullptr);

    // Print off a hello world message
    printf("Hello from main:  processor %d, out of %d processors\n",
           myTid, world_size);

    // Finalize the MPI environment.
    MPI_Finalize();
}