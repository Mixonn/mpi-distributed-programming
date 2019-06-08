#pragma once
#include <string>
#include <climits>
#include <vector>

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
    void put(Node node_to_add);
    void pop(int tid);
    int get_pos(int tid);
    std::string to_string();
    int get_size();
    Node get(int pos);

private:
    std::vector<Node> vec;
};
