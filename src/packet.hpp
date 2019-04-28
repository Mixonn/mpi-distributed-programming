#pragma once
#include <string>

class Packet {
public:
    Packet(int clock_d, int queue_id, int tid, int request_type);
    std::string to_string();

    int clock_d;
    int queue_id;
    int tid;
    int request_type;

};
