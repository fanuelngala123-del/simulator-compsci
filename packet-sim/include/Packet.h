// include/Packet.h
#pragma once

#include <cstdint>

struct Packet {
    double arrival_time = 0.0;
    double service_time = 0.0;      // drawn once at arrival
    int priority = 0;               // 0 = highest, higher number = lower priority
    uint64_t id = 0;

    Packet() = default;
    Packet(double arr_time, double serv_time, int prio, uint64_t pkt_id)
        : arrival_time(arr_time), service_time(serv_time), priority(prio), id(pkt_id) {}
};