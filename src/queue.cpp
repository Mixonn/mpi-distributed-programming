#include "queue.hpp"
#include <vector>
#include <algorithm>
#include <iostream>
#include <cassert>


Node::Node(int primary, int clk, int tid) : primary(primary), clk(clk), tid(tid), next_node(nullptr) { }

std::string Node::to_string() {
    return "clk: " + std::to_string(clk) + " tid: " + std::to_string(tid);
}

bool Node::operator<(const Node &to_compare) const {
    if(primary > to_compare.primary)
        return true;
    if (clk < to_compare.clk)
        return true;
    if (to_compare.clk < clk)
        return false;
    return tid < to_compare.tid;
}

bool Node::operator==(const Node &to_compare) const {
    return (primary == to_compare.primary && clk == to_compare.clk && tid == to_compare.tid);
}

PriorityQueue::PriorityQueue() { }


void PriorityQueue::put(Node node_to_add) {
    vec.push_back(node_to_add);
    std::sort(vec.begin(), vec.end());
}

void PriorityQueue::pop(int tid) { //todo clear nodes from memory
    int pos = get_pos(tid);
    assert(pos != -1);

    vec.erase(vec.begin() + pos);
}

int PriorityQueue::get_pos(int tid) {
    int pos = -1;
    for(int i=0; i<(int)vec.size(); ++i) {
        if(vec[i].tid == tid) {
            pos = i;
            break;
        }
    }

    return pos;
}

std::string PriorityQueue::to_string() {
    std::string result = "Size: " + std::to_string(get_size()) + "\n";
    for(auto &x: vec) result += x.to_string() + "\n";
    return result;
}

int PriorityQueue::get_size() {
    return vec.size();
}

