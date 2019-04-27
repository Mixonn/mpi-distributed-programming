#include <mpi.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <string>
#include <iostream>
#include <random>
#include <utility>
#include <map>

#include "queue.cpp"
#include "packet.cpp"
#include "event.cpp"

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
int myTid;
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
    printf("[ID:%d][CLK:%d]\tSend message to [%d], ID: %d, reqType: %d\n",
           myTid, sent_clk, receiver, queueId, requestType);
    MPI_Request req;
    MPI_Isend(vector, 5, MPI_INT, receiver, receiver, MPI_COMM_WORLD, &req);
}

void send_to_all(int queueId, int requestType, int sent_clk) {
    for (int i = 0; i < world_size; i++) {
        if(i == myTid){
            continue;
        }
        send_to_tid(i, queueId, requestType, sent_clk);
    }
}

void *send_loop(void *id) {
    while (true) {
        pthread_mutex_lock(&workshop_mutex[0]); //add me to queue
        Node node;
        node.tid = myTid;
        node.clk = clock_d;
        workshops[0].queue.put(&node);
        pthread_mutex_unlock(&workshop_mutex[0]);

        send_to_all(0, REQUEST_GET_WS, clock_d);

        pthread_mutex_lock(&mutexClock);
        clock_d++;
        pthread_mutex_unlock(&mutexClock);

        sem_wait(&semaphore); //waiting for all responses

	std::cout << myTid << "RECEIVED ALL RESPONSES" << std::endl;
        sleep(5);
    }
}


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
        pthread_mutex_init(&workshop_mutex[i], nullptr);
    }

    sem_init(&semaphore, 1, 1);
    sem_wait(&semaphore);

    pthread_t threadId;
    pthread_create(&threadId, &attr, send_loop, nullptr);

    auto *packet = new Packet();

    while (true) {
        receive(packet);
        int const queueId = packet->queueId;
        if(packet->requestType == REQUEST_GET_WS){
            pthread_mutex_lock(&workshop_mutex[queueId]);
            Node *node = new Node;
            node->tid = packet->tid;
            node->clk = packet->clock_d;
            workshops[queueId].queue.put(node);
            pthread_mutex_unlock(&workshop_mutex[queueId]);

            pthread_mutex_lock(&mutexClock);
            send_to_tid(packet->tid, queueId, ACCEPT_GET_WS, clock_d);
            clock_d++;
            pthread_mutex_unlock(&mutexClock);
        }
        if(packet->requestType == ACCEPT_GET_WS){
            pthread_mutex_lock(&workshop_mutex[queueId]);
            workshops[queueId].accepted_counter++;
            pthread_mutex_unlock(&workshop_mutex[queueId]);
        }
        int queueSize = workshops[queueId].queue.get_size();
        int queuePos = workshops[queueId].queue.get_pos(myTid);
        int ahead_of = queueSize - queuePos - 1;
        if(ahead_of >= world_size - PYRKON_CAPABILITY && workshops[queueId].accepted_counter == world_size - 1){
            sem_post(&semaphore);
        }
    }

    // Finalize the MPI environment.
    delete packet;
    MPI_Finalize();
}
