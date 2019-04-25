#include <mpi.h> //with openmpi use #include "mpi.h"
#include <unistd.h> //for sleep
#include <pthread.h>
#include <semaphore.h>
#include <string>
#include <iostream>
#include "Queue.cpp"
#include "Packet.cpp"

pthread_mutex_t mutex;
pthread_mutex_t mutexClock;
sem_t semaphore;

int clock_d = 0;
int myTid;
int world_size;

void recive(Packet *packet){
    MPI_Status status;
    int vector[5];

    MPI_Status req;
    MPI_Recv(vector, 5, MPI_INT, MPI_ANY_SOURCE, myTid, MPI_COMM_WORLD, &req);

    pthread_mutex_lock(&mutexClock);
    if (packet->clock_d > clock_d){
        clock_d = packet->clock_d;
    }
    clock_d++;
    pthread_mutex_unlock(&mutexClock);

    packet->clock_d = vector[0];
    packet->type = vector[1];
    packet->message = vector[2];
    packet->tid = vector[3];
    packet->requestType = vector[4];

    printf("[%d][%d]\tRecived message: %s\n",
           myTid, clock_d-1, packet->to_string().c_str());
}

void send_to_tid(int receiver, int type, int message, int requestType, int setClock){
    int vector[5] = {setClock, type, message, myTid, requestType};
    MPI_Request req;
    MPI_Isend(vector, 5, MPI_INT, receiver, receiver, MPI_COMM_WORLD, &req);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
void* send_loop(void *id){
    while(true){
        send_to_tid(myTid, 1, myTid, 1, clock_d);

        pthread_mutex_lock(&mutexClock);
        clock_d++;
        pthread_mutex_unlock(&mutexClock);

        printf("[%d][%d]\tSend message to [%d]\n",
               myTid, clock_d, myTid);
        sleep(5);
    }
}
#pragma clang diagnostic pop

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

    auto* packet = new Packet();

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
    while(true) {
        recive(packet);
    }
#pragma clang diagnostic pop

    // Finalize the MPI environment.
    delete packet;
    MPI_Finalize();
}