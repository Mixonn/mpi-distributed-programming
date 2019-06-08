#include "mpi.h"
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <string>
#include <random>
#include <utility>
#include <cmath>
#include <vector>
#include <map>
#include <thread>
#include <cassert>

#include "queue.hpp"
#include "packet.hpp"
#include "event.hpp"

// Default values, may be provided as command-line args
int workshops_count;
int workshops_capability;
int pyrkon_capability;

const int REQUEST_GET_WS = 133;
const int ACCEPT_GET_WS = 134;
const int REQUEST_LOSE_WS = 233;

std::vector<pthread_mutex_t> workshop_mutex;
std::vector<sem_t> workshop_semaphore;
pthread_mutex_t mutex_clock;

std::vector<Event> workshops;
std::map<int, bool> workshops_to_visit; //todo: in the future we can mark it as <Event, bool>
int clock_d = 1;
int my_tid;
int world_size;


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



void reset_workshops_to_visit() {
    workshops_to_visit.clear();
    // workshops_to_visit.insert(std::make_pair(0, false));
    int to_visit = 1; // TODO: change to get_random_number(1, workshops_count-1), handle workshops_count=1 case
    while(to_visit--) {
        int random_number;
        do {
            random_number = get_random_number(1, workshops_count);
        } while(workshops_to_visit.find(random_number) != workshops_to_visit.end());
        workshops_to_visit.insert(std::make_pair(random_number, false));
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

    // This should not be reached
    Log::warn(my_tid, clock_d, "Cannot mark workshop as visited because it should not be visited");

}

Packet receive() {
    MPI_Status req;
    int *arr = new int[5];
    for(int i=0; i<5; i++) arr[i] = -2;
    int recv_status = MPI_Recv(arr, 5, MPI_INT, MPI_ANY_SOURCE, my_tid, MPI_COMM_WORLD, &req);
    assert(recv_status == MPI_SUCCESS);
    printf("Received %d %d %d %d\n", arr[0], arr[1], arr[2], arr[3]);

    Packet packet(arr[0], arr[1], arr[2], arr[3]);
    Log::info(my_tid, clock_d, "Received " + packet.to_string());
    return packet;
}


void send_to_tid(int receiver, int queue_id, int request_type, int sent_clk) {
    int *arr = new int[5];
    arr[0] = sent_clk;
    arr[1] = queue_id;
    arr[2] = my_tid;
    arr[3] = request_type;

    MPI_Request req;
    int status_send = MPI_Send(arr, 5, MPI_INT, receiver, receiver, MPI_COMM_WORLD);
    assert(status_send == MPI_SUCCESS);

    Packet packet(sent_clk, queue_id, my_tid, request_type);
    Log::info(my_tid, sent_clk, "\tSent to " + std::to_string(receiver) + ": " + packet.to_string());
}

void send_to_all(int queue_id, int request_type, int sent_clk) {
    for (int i = 0; i < world_size; i++) {
        // we are sending request even to this same process because we need notify that queue changed
        send_to_tid(i, queue_id, request_type, sent_clk);
    }
}

void inside_workshop(int workshop_id) {
    pthread_mutex_lock(&mutex_clock);
    {
        Log::info(my_tid, clock_d, "Inside workshop " + std::to_string(workshop_id));
    } pthread_mutex_unlock(&mutex_clock);

    sleep(5);

    pthread_mutex_lock(&mutex_clock);
    {
        Log::info(my_tid, clock_d, "Quitting workshop " + std::to_string(workshop_id));
        send_to_all(workshop_id, REQUEST_LOSE_WS, clock_d);
    } pthread_mutex_unlock(&mutex_clock);
}

void *send_loop(void *id) {
    while (true) {
        pthread_mutex_lock(&workshop_mutex[0]); //add me to queue
        {
            Node node(1, my_tid, clock_d);
            workshops[0].queue.put(node);
        } pthread_mutex_unlock(&workshop_mutex[0]);

        pthread_mutex_lock(&mutex_clock); //chemy wejść do pytkona
        {
            send_to_all(0, REQUEST_GET_WS, clock_d);
            clock_d++;
        } pthread_mutex_unlock(&mutex_clock);

        // Wait for all responses regarding Pyrkon event
        sem_wait(&workshop_semaphore[0]);

        pthread_mutex_lock(&mutex_clock);
        {
            Log::color_info(my_tid, clock_d, "I've got a Pyrkon ticket!", ANSI_COLOR_MAGENTA);
        } pthread_mutex_unlock(&mutex_clock);

        reset_workshops_to_visit();
        std::string drawn_workshops;
        for (auto & it : workshops_to_visit) {
            drawn_workshops += std::to_string(it.first) + ", ";
        }
        pthread_mutex_lock(&mutex_clock);
        {
            Log::debug(my_tid, clock_d, "Drawn workshops: " + drawn_workshops);
            for (auto &it : workshops_to_visit) {
                send_to_all(it.first, REQUEST_GET_WS, clock_d);
            }
            clock_d++;
        } pthread_mutex_unlock(&mutex_clock);


        for(int i=0; i<(int)drawn_workshops.size(); ++i) {
            sem_wait(&workshop_semaphore[i]);

            int ws_to_visit = -1;
            for(auto &it: workshops_to_visit) {
                if(it.second) {
                    ws_to_visit = it.first;
                    break;
                }
            }

            if (ws_to_visit != -1) {
                inside_workshop(ws_to_visit);

                pthread_mutex_lock(&mutex_clock);
                {
                    send_to_all(ws_to_visit, REQUEST_LOSE_WS, clock_d);
                } pthread_mutex_lock(&mutex_clock);
            }

        }

        sleep(300); // TODO: detect the end of Pyrkon
    }
}


int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    if(argc == 1) {
        // Set default parameters
        workshops_count = 10;
        workshops_capability = 1;
        pyrkon_capability = 2;
    } else if(argc == 4) {
        workshops_count = atoi(argv[1]);
        workshops_capability = atoi(argv[2]);
        pyrkon_capability = atoi(argv[3]);
    } else {
        printf("Podaj 3 argumenty oznaczające liczbę warsztatów, pojemność warsztatów i pojemność Pyrkona\n");
	return 1;
    }
    printf("%d %d %d\n", workshops_count, workshops_capability, pyrkon_capability);

    workshop_mutex.resize(workshops_count+1);
    workshop_semaphore.resize(workshops_count+1);
    workshops.resize(workshops_count+1);

    // Get the number of processes
    MPI_Comm_rank(MPI_COMM_WORLD, &my_tid);        /* get current process id */
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_mutex_init(&mutex_clock, nullptr);

    for(int i = 0; i <= workshops_count; i++) {
        workshops[i].id = i;
        pthread_mutex_init(&workshop_mutex[i], nullptr);
        sem_init(&workshop_semaphore[i], 0, 1);
        sem_wait(&workshop_semaphore[i]);
    }

    // Handle send_loop inside seperate thread
    pthread_t threadId;
    pthread_create(&threadId, &attr, send_loop, nullptr);

    while (true) {
        Packet packet = receive();
        const int queue_id = packet.queue_id;
        if(packet.tid != my_tid) {
            pthread_mutex_lock(&mutex_clock);
            {
                clock_d = std::max(clock_d, packet.clock_d) + 1;
            } pthread_mutex_unlock(&mutex_clock);

            if (packet.request_type == REQUEST_GET_WS) {
		Node node(-1, -1, -1); // Will get initialized in the mutex
                pthread_mutex_lock(&mutex_clock);
                {
                    node = Node(1, packet.tid, packet.clock_d);
                } pthread_mutex_unlock(&mutex_clock);

                pthread_mutex_lock(&workshop_mutex[queue_id]);
                    workshops[queue_id].queue.put(node);
                pthread_mutex_unlock(&workshop_mutex[queue_id]);

                pthread_mutex_lock(&mutex_clock);
                {
                    send_to_tid(packet.tid, queue_id, ACCEPT_GET_WS, clock_d);
                    clock_d++;
                } pthread_mutex_unlock(&mutex_clock);
            }
            if (packet.request_type == ACCEPT_GET_WS) {
                pthread_mutex_lock(&workshop_mutex[queue_id]);
                {
                    workshops[queue_id].accepted_counter++;
                    if(workshops_to_visit.find(queue_id) != workshops_to_visit.end()) {
                        workshops_to_visit[queue_id] = true;
                    }
                } pthread_mutex_unlock(&workshop_mutex[queue_id]);
            }
            if (packet.request_type == REQUEST_LOSE_WS){
                pthread_mutex_lock(&workshop_mutex[queue_id]);
                {
                    workshops[queue_id].queue.pop(packet.tid);

                    Node node(0, packet.tid, packet.clock_d);
                    workshops[queue_id].queue.put(node);
                } pthread_mutex_unlock(&workshop_mutex[queue_id]);
            }
        }

        int ahead_of;
        pthread_mutex_lock(&workshop_mutex[queue_id]);
        {
            int queueSize = workshops[queue_id].queue.get_size();
            int queuePos = workshops[queue_id].queue.get_pos(my_tid);
            ahead_of = queueSize - queuePos - 1;

            int capability = (queue_id == 0) ? pyrkon_capability : workshops_capability;
            if (ahead_of >= world_size - capability) {
                sem_post(&workshop_semaphore[queue_id]);
            }
        }
        pthread_mutex_unlock(&workshop_mutex[queue_id]);
    }

    // Finalize the MPI environment.
    MPI_Finalize();
}
