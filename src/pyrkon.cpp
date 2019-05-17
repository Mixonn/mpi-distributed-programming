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
const int WORKSHOPS_CAPABILITY = 1;
const int PYRKON_CAPABILITY = 2;

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
sem_t workshop_semaphore[WORKSHOPS_COUNT+1];

int clock_d = 1;
int my_tid;
int world_size;
Event workshops[WORKSHOPS_COUNT+1];
std::map<int, bool> workshops_to_visit; //todo: in the future we can mark it as <Event, bool>

void reset_workshops_to_visit() {
    workshops_to_visit.clear();
    workshops_to_visit.insert(std::make_pair(0, false));
    int toVisit = 1;
//    int toVisit = get_random_number(1, 5);
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

void mark_workshop_visited(int workshop_id){
    for(auto & it : workshops_to_visit){
        if(it.first != workshop_id){
            continue;
        }
        it.second = true;
        return;
    }
    Log::warn(my_tid, clock_d, "Cannot mark workshop as visited because it should not be visited");

}

Packet receive() {
    MPI_Status req;
    int arr[5];
    memset(arr, -1, sizeof(arr));
    MPI_Recv(arr, 5, MPI_INT, MPI_ANY_SOURCE, my_tid, MPI_COMM_WORLD, &req);

    Packet packet(arr[0], arr[1], arr[2], arr[3]);
    Log::info(my_tid, clock_d, "Received " + packet.to_string());
    return packet;
}


void send_to_tid(int receiver, int queue_id, int request_type, int sent_clk) {
    int arr[] = {sent_clk, queue_id, my_tid, request_type, 0};
    MPI_Request req;

    MPI_Isend(arr, 5, MPI_INT, receiver, receiver, MPI_COMM_WORLD, &req);

    Packet packet(sent_clk, queue_id, my_tid, request_type);
    Log::info(my_tid, sent_clk, "\tSent to " + std::to_string(receiver) + ": " + packet.to_string());
}

void send_to_all(int queue_id, int request_type, int sent_clk) {
    for (int i = 0; i < world_size; i++) {
        // we are sending request even to this same process because we need notify that queue changed
        send_to_tid(i, queue_id, request_type, sent_clk);
    }
}

void *send_loop(void *id) {
    while (true) {

        pthread_mutex_lock(&workshop_mutex[0]); //add me to queue
        {
            Node node(1, my_tid, clock_d);
            workshops[0].queue.put(&node);
        } pthread_mutex_unlock(&workshop_mutex[0]);

        pthread_mutex_lock(&mutexClock);
        {
            send_to_all(0, REQUEST_GET_WS, clock_d);
            clock_d++;
            Log::color_info(my_tid, clock_d, "Received all responses", ANSI_COLOR_MAGENTA);
        } pthread_mutex_unlock(&mutexClock);

        sem_wait(&workshop_semaphore[0]); //waiting for all responses

        reset_workshops_to_visit();
        std::string drawn_workshops;
        for (auto & it : workshops_to_visit)
        {
            drawn_workshops += std::to_string(it.first) + ", ";
        }
        pthread_mutex_lock(&mutexClock);
        Log::debug(my_tid, clock_d, "Drawn workshops: " + drawn_workshops);
        for (auto & it : workshops_to_visit)
        {
            send_to_all(it.first,REQUEST_GET_WS, clock_d);
            clock_d++;
        }
        pthread_mutex_unlock(&mutexClock);
        for (int i=1; i<=WORKSHOPS_COUNT; i++){
            sem_wait(&workshop_semaphore[i]);
        }
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
        sem_init(&workshop_semaphore[i], 0, 1);
        sem_wait(&workshop_semaphore[i]);
    }

    pthread_t threadId;
    pthread_create(&threadId, &attr, send_loop, nullptr);

    while (true) {
        Packet packet = receive();
        int const queue_id = packet.queue_id;
        if(packet.tid != my_tid) {
            pthread_mutex_lock(&mutexClock);
            {
                clock_d = std::max(clock_d, packet.clock_d) + 1;
            } pthread_mutex_unlock(&mutexClock);

            if (packet.request_type == REQUEST_GET_WS) {
                pthread_mutex_lock(&workshop_mutex[queue_id]);
                {
                    Node *node = new Node(1, packet.tid, packet.clock_d);
                    workshops[queue_id].queue.put(node);
                } pthread_mutex_unlock(&workshop_mutex[queue_id]);

                pthread_mutex_lock(&mutexClock);
                {
                    send_to_tid(packet.tid, queue_id, ACCEPT_GET_WS, clock_d);
                    clock_d++;
                } pthread_mutex_unlock(&mutexClock);
            }
            if (packet.request_type == ACCEPT_GET_WS) {
                pthread_mutex_lock(&workshop_mutex[queue_id]);
                {
                    workshops[queue_id].accepted_counter++;
                } pthread_mutex_unlock(&workshop_mutex[queue_id]);
            }
            if (packet.request_type == REQUEST_LOSE_WS){
                pthread_mutex_lock(&workshop_mutex[queue_id]);
                {
                    workshops[queue_id].queue.pop(packet.tid);

                    Node *node = new Node(0, packet.tid, packet.clock_d);
                    workshops[queue_id].queue.put(node);
                } pthread_mutex_unlock(&workshop_mutex[queue_id]);
            }
        }

        int ahead_of;
        int accepted_counter;
        pthread_mutex_lock(&workshop_mutex[queue_id]);
        {
            int queueSize = workshops[queue_id].queue.get_size();
            int queuePos = workshops[queue_id].queue.get_pos(my_tid);
            ahead_of = queueSize - queuePos - 1;
            accepted_counter = workshops[queue_id].accepted_counter;
        } pthread_mutex_unlock(&workshop_mutex[queue_id]);

        int capability = (packet.queue_id == 0) ? PYRKON_CAPABILITY : WORKSHOPS_CAPABILITY;
        if(ahead_of >= world_size - capability){
            sem_post(&workshop_semaphore[queue_id]); //todo it unlocking many times
        }
    }

    // Finalize the MPI environment.
    MPI_Finalize();
}
