// include/Simulator.h
#pragma once

#include "Queue.h"
#include "Packet.h"

#include <random>
#include <vector>
#include <string>
#include <cstdint>
#include <optional>
#include <limits>           // for std::numeric_limits

struct SimulationConfig {
    double lambda;              // arrival rate (packets per second)
    double mu;                  // service rate (packets per second)
    size_t buffer_size;         // max packets in queue
    int num_priorities = 1;     // number of priority levels (must match priority_probs.size())
    double simulation_time;     // total simulation duration (seconds)
    uint64_t random_seed = 42;  // for reproducibility
    bool collect_queue_trace = false;  // whether to record queue length over time

    // Configurable probability distribution for priorities
    // Index 0 = highest priority, last index = lowest priority
    // Values should sum to ~1.0 (will be used as-is, no automatic normalization)
    std::vector<double> priority_probs = {0.50, 0.35, 0.15};
};

struct SimulationStats {
    uint64_t total_arrivals = 0;
    uint64_t total_drops = 0;
    uint64_t total_departures = 0;
    double simulation_duration = 0.0;

    double total_service_time = 0.0;     // sum of service times of departed packets
    double total_waiting_time = 0.0;     // sum of (departure - arrival - service_time)
    double total_system_time = 0.0;      // sum of (departure - arrival)

    size_t max_queue_length = 0;
    double sum_queue_length_samples = 0.0;
    size_t queue_length_sample_count = 0;

    // Optional: trace of queue length over time (only if enabled)
    std::vector<std::pair<double, size_t>> queue_length_trace;

    // Per-priority statistics
    std::vector<uint64_t> arrivals_per_priority;          // arrivals per priority
    std::vector<uint64_t> drops_per_priority;             // drops per priority
    std::vector<uint64_t> departures_per_priority;        // successful departures per priority
    std::vector<double>   total_waiting_time_per_priority; // sum of waiting times per priority

    // Computed averages (global)
    double avg_throughput() const {
        return total_departures > 0 ? total_departures / simulation_duration : 0.0;
    }
    
    double avg_queue_length() const {
        return queue_length_sample_count > 0 
            ? sum_queue_length_samples / queue_length_sample_count 
            : 0.0;
    }
    
    double avg_waiting_time() const {
        return total_departures > 0 ? total_waiting_time / total_departures : 0.0;
    }
    
    double avg_system_time() const {
        return total_departures > 0 ? total_system_time / total_departures : 0.0;
    }
    
    double drop_probability() const {
        return total_arrivals > 0 ? static_cast<double>(total_drops) / total_arrivals : 0.0;
    }

    // Optional: per-priority helpers
    double drop_probability(int priority) const {
        if (priority < 0 || static_cast<size_t>(priority) >= arrivals_per_priority.size()) return 0.0;
        auto arrivals = arrivals_per_priority[priority];
        return arrivals > 0 ? static_cast<double>(drops_per_priority[priority]) / arrivals : 0.0;
    }

    double avg_waiting_time(int priority) const {
        if (priority < 0 || static_cast<size_t>(priority) >= departures_per_priority.size()) return 0.0;
        auto departs = departures_per_priority[priority];
        return departs > 0 ? total_waiting_time_per_priority[priority] / departs : 0.0;
    }
};

class Simulator {
public:
    explicit Simulator(const SimulationConfig& config);
    ~Simulator() = default;

    // Set the queue to use (ownership remains with caller)
    void set_queue(Queue* queue);

    // Run the full simulation
    void run();

    // Get the results after simulation
    const SimulationStats& get_stats() const { return stats_; }

    // Optional: get a short summary string
    std::string summary() const;

private:
    // Configuration
    SimulationConfig config_;

    // Random number generation
    std::mt19937 rng_;
    std::exponential_distribution<double> interarrival_dist_;
    std::exponential_distribution<double> service_dist_;

    // Simulation state
    double current_time_ = 0.0;
    double next_arrival_time_ = 0.0;
    double next_departure_time_ = std::numeric_limits<double>::infinity();

    bool server_busy_ = false;
    std::optional<Packet> current_serving_packet_;

    // Queue (not owned by simulator)
    Queue* queue_ = nullptr;

    // Packet counter
    uint64_t next_packet_id_ = 1;

    // Statistics
    SimulationStats stats_;

    // Internal methods
    void schedule_next_arrival();
    void process_arrival();
    void process_departure();
    void update_queue_length_stats();
    double get_next_event_time() const;
    void start_service(const Packet& packet);
};