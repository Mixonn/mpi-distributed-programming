#include <mpi.h> //with openmpi use #include "mpi.h"
#include <unistd.h> //for sleep
#include <pthread.h>
#include <semaphore.h>
#include <string>
#include <iostream>
#include <random>
#include <utility> //for pair
#include <map>

#include "Queue.cpp"
#include "Packet.cpp"
#include "Event.cpp"

#define WORKSHOPS_COUNT 10
#define WORKSHOPS_CAPABILITY 2
#define PYRKON_CAPABILITY 2

#define REQUEST_GET 100
#define REQUEST_LOSE 200


/**
 * Return random number. The left and right margin inclusive.
 * @param min minimum value (inclusive)
 * @param max maximum value (inclusive)
 * @return return random value between min and max inclusive
 */
int get_random_number(int min, int max){
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<double> dist(min, max+1);
    return (int)dist(mt);
}

pthread_mutex_t workshop_mutex[WORKSHOPS_COUNT+1];
pthread_mutex_t mutexClock;
sem_t semaphore;

int clock_d = 1;
int myTid;
int world_size;
Event workshops[WORKSHOPS_COUNT+1];
std::map<int, bool> workshops_to_visit;

void reset_workshops_to_visit(){
    workshops_to_visit.clear();
    int toVisit = get_random_number(1, 5);
    while(toVisit--){
        int rand = get_random_number(1, WORKSHOPS_COUNT);
        while(workshops_to_visit.find(rand) != workshops_to_visit.end()){
            rand = get_random_number(1, WORKSHOPS_COUNT);
        }
        workshops_to_visit.insert(std::make_pair(rand, false));
    }
}

void receive(Packet *packet) {
    MPI_Status status;
    int vector[5];

    MPI_Status req;
    MPI_Recv(vector, 5, MPI_INT, MPI_ANY_SOURCE, myTid, MPI_COMM_WORLD, &req);

    pthread_mutex_lock(&mutexClock);
    if (packet->clock_d > clock_d) {
        clock_d = packet->clock_d;
    }
    clock_d++;
    pthread_mutex_unlock(&mutexClock);

    packet->clock_d = vector[0];
    packet->queueId = vector[1];
    packet->tid = vector[2];
    packet->requestType = vector[3];

    printf("[ID:%d][CLK:%d]\tRecived message: %s\n",
           myTid, clock_d - 1, packet->to_string().c_str());
}

void send_to_tid(int receiver, int queueId, int requestType, int sent_clk) {
    int vector[5] = {sent_clk, queueId, myTid, requestType};
    printf("[ID:%d][CLK:%d]\tSend message to [%d]\n",
           myTid, sent_clk, receiver);
    MPI_Request req;
    MPI_Isend(vector, 5, MPI_INT, receiver, receiver, MPI_COMM_WORLD, &req);
}

void send_to_all(int queueId, int requestType, int sent_clk) {
    for (int i = 0; i < world_size; i++) {
        send_to_tid(i, queueId, requestType, sent_clk);
    }
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
void *send_loop(void *id) {
    while (true) {
        send_to_all(1, 1, clock_d);

        pthread_mutex_lock(&mutexClock);
        clock_d++;

        pthread_mutex_unlock(&mutexClock);
        sleep(5);
    }
}
#pragma clang diagnostic pop

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    // Get the number of processes
    MPI_Comm_rank(MPI_COMM_WORLD, &myTid);        /* get current process id */
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_mutex_init(&mutexClock, nullptr);

    for(int i = 0; i < WORKSHOPS_COUNT; i++){
        workshops[i].id = i;
        if(i == 0) { //pyrkon
            workshops[i].queue.setMaxSize(PYRKON_CAPABILITY);
        }
        workshops[i].queue.setMaxSize(WORKSHOPS_CAPABILITY);
        pthread_mutex_init(&workshop_mutex[i], nullptr);
    }

    sem_init(&semaphore, 1, 1);
    sem_wait(&semaphore);

    pthread_t threadId;
    pthread_create(&threadId, &attr, send_loop, nullptr);

    auto *packet = new Packet();

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
    while (true) {
        receive(packet);
    }
#pragma clang diagnostic pop

    // Finalize the MPI environment.
    delete packet;
    MPI_Finalize();
}