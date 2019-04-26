#pragma once
#include <string>

class Packet {
public:
    int clock_d;
    int type;
    int message;
    int tid;
    int requestType;

    Packet() = default;

    Packet(int clockD, int type, int message, int tid, int requestType) : clock_d(clockD), type(type), message(message),
                                                                          tid(tid), requestType(requestType) {};


    std::string to_string() {
        return "clk: " + std::to_string(clock_d) + " type: " + std::to_string(type) + " message: " + std::to_string(message) + " tid: "
               + std::to_string(tid) + " requestType: " + std::to_string(requestType);
    }
};