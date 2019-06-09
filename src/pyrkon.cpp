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
#include <queue>
#include <algorithm>

#include "queue.hpp"
#include "packet.hpp"
#include "event.hpp"

// Default values, may be provided as command-line args
int workshops_count;
int workshops_capability;
int pyrkon_capability;

const int REQUEST_GET_WS = 133;
const int ACCEPT_GET_WS_REFUSE = 135;
const int REQUEST_LOSE_WS = 233;

std::vector<pthread_mutex_t> workshop_mutex;
sem_t pyrkon_semaphore, workshop_semaphore;
pthread_mutex_t clock_mutex;

std::vector<Event> workshops;
std::vector<int> drawn_workshops;
std::queue<int> queued_workshops;
std::vector<bool> visited;
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



void reset_drawn_workshops() {
    drawn_workshops.clear();
    int was_visited = 1; // TODO: change to get_random_number(1, workshops_count-1), handle workshops_count=1 case
    while(was_visited--) {
        int random_number;
        do {
            random_number = get_random_number(1, workshops_count);
        } while(std::find(drawn_workshops.begin(), drawn_workshops.end(), random_number) != drawn_workshops.end());
        drawn_workshops.push_back(random_number);
    }
}

Packet receive() {
    MPI_Status req;
    int arr[5] = { -2, -2, -2, -2, -2};
    int recv_status = MPI_Recv(arr, 5, MPI_INT, MPI_ANY_SOURCE, my_tid, MPI_COMM_WORLD, &req);
    assert(recv_status == MPI_SUCCESS);

    Packet packet(arr[0], arr[1], arr[2], arr[3]);
    return packet;
}


void send_to_tid(int receiver, int queue_id, int request_type, int sent_clk) {
    /* Requires locking clock_mutex */ 
    clock_d++;

    int arr[] = {sent_clk, queue_id, my_tid, request_type, 0};

    MPI_Request req;
    int status_send = MPI_Send(arr, 5, MPI_INT, receiver, receiver, MPI_COMM_WORLD);
    assert(status_send == MPI_SUCCESS);

    Packet packet(sent_clk, queue_id, my_tid, request_type);
    Log::info(my_tid, sent_clk, "\tSent to " + std::to_string(receiver) + ": " + packet.to_string());
}

void send_to_all(int queue_id, int request_type, int sent_clk) {
    /* Requires locking clock_mutex */
    for (int i = 0; i < world_size; i++) {
        // we are sending request even to this same process because we need notify that queue changed
        send_to_tid(i, queue_id, request_type, sent_clk);
    }
}

void inside_workshop(int workshop_id) {
    pthread_mutex_lock(&clock_mutex);
    {
        Log::info(my_tid, clock_d, "Inside workshop " + std::to_string(workshop_id));
    } pthread_mutex_unlock(&clock_mutex);

    sleep(5);
}

void *send_loop(void *id) {
    while (true) {
        pthread_mutex_lock(&workshop_mutex[0]); //add me to queue
        {
            Node node(1, clock_d, my_tid);
            workshops[0].queue.put(node);
        } pthread_mutex_unlock(&workshop_mutex[0]);

        pthread_mutex_lock(&clock_mutex);
        {
            send_to_all(0, REQUEST_GET_WS, clock_d);
        } pthread_mutex_unlock(&clock_mutex);

        // Wait for all responses regarding Pyrkon event
        sem_wait(&pyrkon_semaphore);

        pthread_mutex_lock(&clock_mutex);
        {
            Log::color_info(my_tid, clock_d, "I've got a Pyrkon ticket!", ANSI_COLOR_MAGENTA);
        } pthread_mutex_unlock(&clock_mutex);

        reset_drawn_workshops();
        std::string drawn_workshops_str;
        for (auto & it : drawn_workshops) {
            drawn_workshops_str += std::to_string(it) + ", ";
        }
        pthread_mutex_lock(&clock_mutex);
        {
            Log::debug(my_tid, clock_d, "Drawn workshops: " + drawn_workshops_str);
            for (int i = 1; i <= workshops_count; i++){
                if (std::find(drawn_workshops.begin(), drawn_workshops.end(), i) == drawn_workshops.end()) {
                    send_to_all(i, ACCEPT_GET_WS_REFUSE, clock_d);
                } else {
                    send_to_all(i, REQUEST_GET_WS, clock_d);
                }
            }
        } pthread_mutex_unlock(&clock_mutex);


	while(true) {
            Log::color_info(my_tid, -1, "I am about to freeze and wait for the workshop", ANSI_COLOR_MAGENTA);
            sem_wait(&workshop_semaphore);
            Log::color_info(my_tid, -1, "I am getting unlocked and will get to the workshop just in a minute", ANSI_COLOR_MAGENTA);

	    if(queued_workshops.empty()) break;
	    int ws_was_visited = queued_workshops.front();
	    queued_workshops.pop();

            inside_workshop(ws_was_visited);

            pthread_mutex_lock(&clock_mutex);
            {
                send_to_all(ws_was_visited, REQUEST_LOSE_WS, clock_d);
            } pthread_mutex_unlock(&clock_mutex);
        }

        pthread_mutex_lock(&clock_mutex);
        {
            send_to_all(0, REQUEST_LOSE_WS, clock_d);
	    Log::info(my_tid, clock_d, "Finishing Pyrkon!");
        } pthread_mutex_unlock(&clock_mutex);
	return 0; // Finish Pyrkon TODO: implement never-ending Pyrkon
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
    workshops.resize(workshops_count+1);
    visited.resize(workshops_count+1);

    // Get the number of processes
    MPI_Comm_rank(MPI_COMM_WORLD, &my_tid);        /* get current process id */
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_mutex_init(&clock_mutex, nullptr);

    sem_init(&pyrkon_semaphore, 0, 0);
    sem_init(&workshop_semaphore, 0, 0);

    for(int i = 0; i <= workshops_count; i++) {
        workshops[i].id = i;
        pthread_mutex_init(&workshop_mutex[i], nullptr);
    }

    // Handle send_loop inside seperate thread
    pthread_t threadId;
    pthread_create(&threadId, &attr, send_loop, nullptr);

    while (true) {
        Packet packet = receive();
        const int queue_id = packet.queue_id;

	if(my_tid != packet.tid) {
            pthread_mutex_lock(&clock_mutex);
            {
                clock_d = std::max(clock_d, packet.clock_d) + 1;
                Log::info(my_tid, clock_d, "Received " + packet.to_string());
            } pthread_mutex_unlock(&clock_mutex);

            if (packet.request_type == REQUEST_GET_WS) {
	        Node node(-1, -1, -1); // Will get initialized in the mutex
                pthread_mutex_lock(&clock_mutex);
                {
                    node = Node(1, packet.clock_d, packet.tid);
                } pthread_mutex_unlock(&clock_mutex);

                pthread_mutex_lock(&workshop_mutex[queue_id]);
	        {
	            Log::info(my_tid, clock_d, "Putting " + node.to_string() + " in " + std::to_string(queue_id) + "-th queue");
                    workshops[queue_id].queue.put(node);
	        } pthread_mutex_unlock(&workshop_mutex[queue_id]);
            }
            if (packet.request_type == REQUEST_LOSE_WS) {
                pthread_mutex_lock(&workshop_mutex[queue_id]);
                {
                    workshops[queue_id].queue.pop(packet.tid);

                    Node node(0, packet.clock_d, packet.tid);
                    workshops[queue_id].queue.put(node);
                } pthread_mutex_unlock(&workshop_mutex[queue_id]);
            }
            
            if(packet.request_type == ACCEPT_GET_WS_REFUSE){
                pthread_mutex_lock(&workshop_mutex[queue_id]);
                {
                    Node node(0, packet.clock_d, packet.tid);
	            Log::info(my_tid, clock_d, "Putting ignored " + node.to_string() + " in " + std::to_string(queue_id) + "-th queue");
                    workshops[queue_id].queue.put(node);
                } pthread_mutex_unlock(&workshop_mutex[queue_id]);
            }

            pthread_mutex_lock(&workshop_mutex[queue_id]);
            {
                int queue_size = workshops[queue_id].queue.get_size();
                int queue_pos = workshops[queue_id].queue.get_pos(my_tid);
                int ahead_of = queue_size - queue_pos - 1;

                int capability = (queue_id == 0) ? pyrkon_capability : workshops_capability;
                int current_world_size = (queue_id == 0) ? world_size : std::min(pyrkon_capability, world_size);
                std::string msg = "Trying to wake up" +
                        std::string(", req type = ") + std::to_string(packet.request_type) +
                        ", queue_pos = " + std::to_string(queue_pos) +
                        ", queue_size = " + std::to_string(queue_size) +
                        ", capability = " + std::to_string(capability) +
                        ", ahead_of = " + std::to_string(ahead_of) +
                        ", request_type = " + std::to_string(packet.request_type) +
	    	    ", visited[queue_id] = " + std::to_string(visited[queue_id]);
                Log::info(my_tid, -1, msg);

                if (queue_pos != -1 && !visited[queue_id] && ahead_of >= current_world_size - capability) {
	    	visited[queue_id] = true;
                    if(queue_id == 0) sem_post(&pyrkon_semaphore);
                    else {
	    	    printf("Dorzucamy do kolejki: %d\n", queue_id);
	    	    queued_workshops.push(queue_id);
                        sem_post(&workshop_semaphore);
                    }
                } else {
                    std::string tmp = "";
                    if(queue_pos == -1){
                        tmp += std::string("queue_pos == -1\t");
                    }
	            tmp += "packet.request_type = " + std::to_string(packet.request_type) + "\t";
                    if(ahead_of < current_world_size - capability){
                        tmp += std::string("ahead_of < world_size - capability\t");
                    }
                    Log::info(my_tid, -1, "Nie wchodzę, bo: " + tmp + ", queue_id " + std::to_string(queue_id) + ", szukamy " + std::to_string(my_tid) + ", KOLEJKA = " + workshops[queue_id].queue.to_string());
                }
            }
            pthread_mutex_unlock(&workshop_mutex[queue_id]);
	}
    }

    // Finalize the MPI environment.
    MPI_Finalize();
}
