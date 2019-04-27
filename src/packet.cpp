#include "packet.hpp"


Packet::Packet(int clockD, int queueId, int tid, int requestType) : clock_d(clockD), queueId(queueId), tid(tid), requestType(requestType) {};


std::string Packet::to_string() {
    return "clk: " + std::to_string(clock_d) + " queueId: " + std::to_string(queueId) + " tid: "
           + std::to_string(tid) + " requestType: " + std::to_string(requestType);
}

