#pragma once
#include <cstring>
#include "Log.cpp"
#include <climits>


using namespace std;

class Node {
public:
    int clk;
    int tid;
    Node *next_node;

    std::string to_string() {
        return "clk: " + std::to_string(clk) + " tid: " + std::to_string(tid);
    }

    bool operator<(const Node &to_compare) const {
        if (clk < to_compare.clk)
            return true;
        if (to_compare.clk < clk)
            return false;
        return tid < to_compare.tid;
    }

    bool operator>(const Node &to_compare) const {
        return to_compare < *this;
    }

    bool operator<=(const Node &to_compare) const {
        return !(to_compare < *this);
    }

    bool operator>=(const Node &to_compare) const {
        return !(*this < to_compare);
    }
};

class Priority_Queue {
private:
    Node *front;
    int size = 0;

public:
    Priority_Queue() {
        front = nullptr;
    }

    ~Priority_Queue() {
        Node *curr = front;
        while (curr != nullptr) {
            Node *next = curr->next_node;
            free(curr); //todo: is this correct?
            curr = next;
        }
    }

    void put(Node *node_to_add) {
        size++;
        if (front == nullptr) {
            front = node_to_add;
            return;
        }
        if (*front > *node_to_add) {
            Node *tmp = front;
            front = node_to_add;
            node_to_add->next_node = tmp;
            return;
        }
        Node *next = front;
        while (next->next_node != nullptr && *(next->next_node) < *node_to_add) {
            next = next->next_node;
        }
        if (next->next_node == nullptr) {
            next->next_node = node_to_add;
        } else {
            Node *tmp = next->next_node;
            next->next_node = node_to_add;
            node_to_add->next_node = tmp;
        }
    }

    void pop(int tid) { //todo clear nodes from memory
        Node *next = front;
        Node *previous = nullptr;

        while (next != nullptr && next->tid != tid) {
            previous = next;
            next = next->next_node;
        }
        if (next == nullptr) {
            Log::error("Cannot find node with " + std::to_string(tid) + " process id");
        } else {
            if (previous == nullptr) {
                front = nullptr;
            } else {
                previous->next_node = next->next_node;
            }
            size--;
        }
    }

    /**
     * Return position of node with passed tid
     * @param tid tid to find
     * @return return position of node in queue (first element position is 0).
     * Return -1 when node not found.
     */
    int get_pos(int tid){
        int pos = 0;
        Node* node = front;
        while(node != nullptr && node->tid != tid){
            node = node->next_node;
            pos ++;
        }
        if(node == nullptr){
            return -1;
        } else {
            return pos;
        }
    }

    std::string to_string() {
        std::string result = "Size: " + std::to_string(size) + "\r\n";
        Node *next = front;
        while (next != nullptr) {
            result.append(next->to_string() + "\r\n");
            next = next->next_node;
        }
        return result;
    }

    int get_size() {
        return size;
    }

    void setFront(Node *new_front) {
        if(size != 0){
            Log::warn("Setting queue front element when queue is not empty");
        }
        Priority_Queue::front = new_front;
    }

    Node *getFront() const {
        return front;
    }
};
