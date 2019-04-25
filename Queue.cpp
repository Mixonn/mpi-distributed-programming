#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>

using namespace std;
struct node {
    int clk;
    int tid;
    struct node *next_node;
};

class Priority_Queue {
private:
    node *front;
    int size = 0;
public:
    Priority_Queue() {
        front = nullptr;
    }

    ~Priority_Queue() {
        node *curr = front;
        while (curr != nullptr) {
            node *next = curr->next_node;
            free(curr); //is this correct?
            curr = next;
        }
    }

    void insert(node *node_to_add) {
        //todo
        size++;
    }

    void remove_node(int tid) {
        //todo
        size--;
    }

    int get_size() {
        return size;
    }
};
