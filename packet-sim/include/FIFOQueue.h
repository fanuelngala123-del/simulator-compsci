// include/FIFOQueue.h
#pragma once

#include "Queue.h"
#include <deque>

class FIFOQueue : public Queue {
private:
    std::deque<Packet> buffer_;
    size_t max_size_;

public:
    explicit FIFOQueue(size_t max_size) : max_size_(max_size) {}

    bool enqueue(const Packet& p) override {
        if (buffer_.size() >= max_size_) {
            return false; // tail drop
        }
        buffer_.push_back(p);
        return true;
    }

    Packet dequeue() override {
        Packet p = buffer_.front();
        buffer_.pop_front();
        return p;
    }

    bool is_empty() const override { return buffer_.empty(); }
    bool is_full() const override { return buffer_.size() >= max_size_; }
    size_t size() const override { return buffer_.size(); }
    size_t capacity() const override { return max_size_; }
};