#pragma once
#include "queue.hpp"


class Event {
public:
    Event();

    int id;
    int accepted_counter;
    PriorityQueue queue;

};
