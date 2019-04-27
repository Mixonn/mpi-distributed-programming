#pragma once
#include "queue.cpp"

class Event {
public:
    int id;
    PriorityQueue queue;
    int accepted_counter = 0;
};
