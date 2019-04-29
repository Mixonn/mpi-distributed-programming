#include <mpi.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <string>
#include <iostream>
#include <random>
#include <utility>
#include <cmath>
#include <vector>
#include <map>

#include "queue.hpp"
#include "packet.hpp"
#include "event.hpp"

const int WORKSHOPS_COUNT = 10;
const int WORKSHOPS_CAPABILITY = 2;
const int PYRKON_CAPABILITY = 10;

const int REQUEST_GET_WS = 133;
const int ACCEPT_GET_WS = 134;
const int REQUEST_LOSE_WS = 233;


/**
 * Return random number. The left and right margin inclusive.
 * @param min minimum value (inclusive)
 * @param max maximum value (inclusive)
 * @return return random value between min and max inclusive
 */
int get_random_number(int min, int max) {
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<double> dist(min, max+1);
    return (int)dist(mt);
}

pthread_mutex_t workshop_mutex[WORKSHOPS_COUNT+1];
pthread_mutex_t mutexClock;
sem_t semaphore;

int clock_d = 1;
int my_tid;
int world_size;
Event workshops[WORKSHOPS_COUNT+1];
std::map<int, bool> workshops_to_visit;

void reset_workshops_to_visit() {
    workshops_to_visit.clear();
    workshops_to_visit.insert(std::make_pair(0, false));
    int toVisit = get_random_number(1, 5);
    while(toVisit--){
        int rand = get_random_number(1, WORKSHOPS_COUNT);
        while(workshops_to_visit.find(rand) != workshops_to_visit.end()){
            rand = get_random_number(1, WORKSHOPS_COUNT);
        }
        workshops_to_visit.insert(std::make_pair(rand, false));
    }
}

bool want_to_visit(int workshop_id){
    return workshops_to_visit.find(workshop_id) != workshops_to_visit.end();
}

Packet receive() {
    MPI_Status req;
    int arr[5];
    MPI_Recv(&arr, 5, MPI_INT, MPI_ANY_SOURCE, my_tid, MPI_COMM_WORLD, &req);

    Packet packet(arr[0], arr[1], arr[2], arr[3]);
    Log::info(my_tid, clock_d, "Received " + packet.to_string());

    pthread_mutex_lock(&mutexClock);
    clock_d = std::max(clock_d, packet.clock_d) + 1;
    pthread_mutex_unlock(&mutexClock);

    return packet;
}


void send_to_tid(int receiver, int queue_id, int request_type, int sent_clk) {
    int arr[] = {sent_clk, queue_id, my_tid, request_type, 0};
    MPI_Request req;

    MPI_Isend(arr, 5, MPI_INT, receiver, receiver, MPI_COMM_WORLD, &req);

    Packet packet(sent_clk, queue_id, my_tid, request_type);
    Log::info(my_tid, sent_clk, "Sent to " + std::to_string(receiver) + ": " + packet.to_string());
}

void send_to_all(int queue_id, int request_type, int sent_clk) {
    for (int i = 0; i < world_size; i++) {
        if(i == my_tid){
            continue;
        }
        send_to_tid(i, queue_id, request_type, sent_clk);
    }
}

void *send_loop(void *id) {
    while (true) {
        pthread_mutex_lock(&workshop_mutex[0]); //add me to queue
        Node node;
        node.tid = my_tid;
        node.clk = clock_d;
        workshops[0].queue.put(&node);
        pthread_mutex_unlock(&workshop_mutex[0]);

        send_to_all(0, REQUEST_GET_WS, clock_d);

        pthread_mutex_lock(&mutexClock);
        clock_d++;
        pthread_mutex_unlock(&mutexClock);

        sem_wait(&semaphore); //waiting for all responses
        Log::color_info(my_tid, clock_d, "Received all responses", ANSI_COLOR_MAGENTA);

//        reset_workshops_to_visit();
//        pthread_mutex_lock(&mutexClock);
//        for (auto & it : workshops_to_visit)
//        {
//            send_to_all(it.first,REQUEST_GET_WS, clock_d);
//            clock_d++;
//
//        }
//        pthread_mutex_unlock(&mutexClock);

        sleep(30);
    }
}


int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    // Get the number of processes
    MPI_Comm_rank(MPI_COMM_WORLD, &my_tid);        /* get current process id */
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_mutex_init(&mutexClock, nullptr);

    for(int i = 0; i <= WORKSHOPS_COUNT; i++){
        workshops[i].id = i;
        pthread_mutex_init(&workshop_mutex[i], nullptr);
    }


    sem_init(&semaphore, 1, 1);
    sem_wait(&semaphore);

    pthread_t threadId;
    pthread_create(&threadId, &attr, send_loop, nullptr);

    while (true) {
        Packet packet = receive();
        int const queue_id = packet.queue_id;
        if(packet.request_type == REQUEST_GET_WS){
            pthread_mutex_lock(&workshop_mutex[queue_id]);
            Node *node = new Node;
            node->tid = packet.tid;
            node->clk = packet.clock_d;
            workshops[queue_id].queue.put(node);
            pthread_mutex_unlock(&workshop_mutex[queue_id]);

            pthread_mutex_lock(&mutexClock);
            send_to_tid(packet.tid, queue_id, ACCEPT_GET_WS, clock_d);
            clock_d++;
            pthread_mutex_unlock(&mutexClock);
        }
        if(packet.request_type == ACCEPT_GET_WS){
            pthread_mutex_lock(&workshop_mutex[queue_id]);
            workshops[queue_id].accepted_counter++;
            pthread_mutex_unlock(&workshop_mutex[queue_id]);
        }
        int queueSize = workshops[queue_id].queue.get_size();
        int queuePos = workshops[queue_id].queue.get_pos(my_tid);
        int ahead_of = queueSize - queuePos - 1;
        if(ahead_of >= world_size - PYRKON_CAPABILITY && workshops[queue_id].accepted_counter == world_size - 1){
            sem_post(&semaphore);
        }
    }

    // Finalize the MPI environment.
    MPI_Finalize();
}
