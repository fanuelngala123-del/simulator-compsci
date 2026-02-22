// include/PriorityQueue.h
#pragma once

#include "Queue.h"
#include "Packet.h"

#include <vector>
#include <deque>
#include <cstddef>
#include <stdexcept>
#include <string>

class PriorityQueue : public Queue {
private:
    // queues_[0] = highest priority, queues_[back] = lowest
    std::vector<std::deque<Packet>> priority_queues_;

    // Total maximum number of packets allowed in the buffer (across all priorities)
    size_t max_buffer_size_;

    // Current total number of packets in all queues
    size_t current_size_ = 0;

public:
    /**
     * @param max_buffer_size   Total buffer capacity in packets
     * @param num_priority_levels Number of distinct priority levels (e.g. 3 or 4)
     *                            Priority 0 = highest, priority (n-1) = lowest
     */
    PriorityQueue(size_t max_buffer_size, size_t num_priority_levels = 4)
        : max_buffer_size_(max_buffer_size),
          priority_queues_(num_priority_levels)
    {
        if (num_priority_levels == 0) {
            throw std::invalid_argument("Number of priority levels must be at least 1");
        }
    }

    // Try to add a packet
    // Returns true if enqueued, false if dropped (buffer full)
    bool enqueue(const Packet& p) override
    {
        if (current_size_ >= max_buffer_size_) {
            return false;  // tail drop - no space
        }

        size_t prio = static_cast<size_t>(p.priority);

        if (prio >= priority_queues_.size()) {
            // Invalid priority → we could map it to lowest, but for now: drop
            return false;
        }

        priority_queues_[prio].push_back(p);
        ++current_size_;
        return true;
    }

    // Remove and return the next packet to serve
    // Always takes from the highest-priority non-empty queue
    Packet dequeue() override
    {
        if (is_empty()) {
            throw std::runtime_error("dequeue called on empty priority queue");
        }

        // Find the highest priority queue that has packets
        for (auto& q : priority_queues_) {
            if (!q.empty()) {
                Packet pkt = std::move(q.front());
                q.pop_front();
                --current_size_;
                return pkt;
            }
        }

        // Should never reach here if is_empty() is correct
        throw std::runtime_error("Priority queue internal inconsistency");
    }

    bool is_empty() const override
    {
        return current_size_ == 0;
    }

    bool is_full() const override
    {
        return current_size_ >= max_buffer_size_;
    }

    size_t size() const override
    {
        return current_size_;
    }

    size_t capacity() const override
    {
        return max_buffer_size_;
    }

    // Useful for statistics and debugging
    size_t size_of_priority(int priority) const
    {
        if (priority < 0 || static_cast<size_t>(priority) >= priority_queues_.size()) {
            return 0;
        }
        return priority_queues_[static_cast<size_t>(priority)].size();
    }

    size_t num_priority_levels() const
    {
        return priority_queues_.size();
    }

    // Optional: for reporting / debugging
    std::string to_string() const
    {
        std::string s = "PriorityQueue [total=" + std::to_string(current_size_) +
                        "/" + std::to_string(max_buffer_size_) + "]  ";
        for (size_t i = 0; i < priority_queues_.size(); ++i) {
            s += "p" + std::to_string(i) + ":" + std::to_string(priority_queues_[i].size()) + " ";
        }
        return s;
    }
};