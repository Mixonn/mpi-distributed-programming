#include "packet.hpp"


Packet::Packet(int clock_d, int queue_id, int tid, int request_type) : clock_d(clock_d), queue_id(queue_id), tid(tid), request_type(request_type) {};


std::string Packet::to_string() {
    return "clk: " + std::to_string(clock_d) + " queue_id: " + std::to_string(queue_id) + " tid: "
           + std::to_string(tid) + " request_type: " + std::to_string(request_type);
}

