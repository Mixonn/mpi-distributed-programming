#pragma once
#include <string>
#include <climits>

#include "log.hpp"


class Node {
public:
    Node(int primary, int clk, int tid);
    std::string to_string();

    bool operator<(const Node &to_compare) const;
    bool operator>(const Node &to_compare) const;
    bool operator==(const Node &to_compare) const;
    bool operator<=(const Node &to_compare) const;
    bool operator>=(const Node &to_compare) const;

    int primary;
    int clk;
    int tid;
    Node *next_node;
};

class PriorityQueue {
public:
    PriorityQueue();
    ~PriorityQueue();
    void put(Node *node_to_add);
    void pop(int tid);
    /**
     * Return position of node with passed tid
     * @param tid tid to find
     * @return return position of node in queue (first element position is 0).
     * Return -1 when node not found.
     */
    int get_pos(int tid);
    std::string to_string();
    int get_size();
    void set_front(Node *new_front);
    Node *getFront() const;

private:
    Node *front;
    int size;

};
