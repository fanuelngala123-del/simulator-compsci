// include/Queue.h
#pragma once

#include "Packet.h"
#include <cstddef>

class Queue {
public:
    virtual ~Queue() = default;

    // Returns true if successfully enqueued, false if dropped
    virtual bool enqueue(const Packet& p) = 0;

    // Assume called only when !empty()
    virtual Packet dequeue() = 0;

    virtual bool is_empty() const = 0;
    virtual bool is_full() const = 0;
    virtual size_t size() const = 0;
    virtual size_t capacity() const = 0;
};