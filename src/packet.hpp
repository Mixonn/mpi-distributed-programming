#pragma once
#include <string>

class Packet {
public:
    Packet() = default;
    Packet(int clockD, int queueId, int tid, int requestType);
    std::string to_string();

    int clock_d;
    int queueId;
    int tid;
    int requestType;

};
