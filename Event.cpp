#include "Queue.cpp"

class Event{
public:
    int id;
    Priority_Queue queue;
    int accepted_counter = 0;
};
