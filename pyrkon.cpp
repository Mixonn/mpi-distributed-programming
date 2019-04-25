#include <mpi.h> //with openmpi use #include "mpi.h"
#include <unistd.h> //for sleep
#include <pthread.h>
#include <semaphore.h>
#include <string>
#include <iostream>
#include "Queue.cpp"

pthread_mutex_t mutex;
pthread_mutex_t mutexClock;
sem_t semaphore;

class Packet {
public:
    int clock_d;
    int type;
    int message;
    int tid;
    int requestType;

    std::string to_string() {
        return "clk: " + std::to_string(clock_d) + " type: " + std::to_string(type) + " message: " + std::to_string(message) + " tid: "
           + std::to_string(tid) + " requestType: " + std::to_string(requestType);
    }
};

class Node {
public:
    int tid;
    int clock_d;
    Node * lesser;
    Node * greater;

    void insert_with_order(Node* node_to_add){

    }
};

Node* head = nullptr;
void add_with_order_to_queue(Node* queue_head, Node* node_to_add) {
    Node* current_node = queue_head;
    while(current_node != nullptr){
        if(current_node->clock_d == node_to_add->clock_d){
            if(current_node->tid < node_to_add->tid){
                Node* tmpNext = current_node->next;
                current_node->next = node_to_add;
                node_to_add->next = tmpNext;
            } else {

            }
        }
    }
    new_node->tid =
    new_node->next = head;
    head = new_node;
}

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

    Packet* packet = new Packet();

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