#include "Simulator.h"

#include <iostream>
#include <iomanip>
#include <limits>
#include <sstream>

Simulator::Simulator(const SimulationConfig& config)
    : config_(config),
      rng_(config.random_seed),
      interarrival_dist_(1.0 / config.lambda),
      service_dist_(1.0 / config.mu)
{
    if (config.lambda <= 0.0 || config.mu <= 0.0) {
        throw std::invalid_argument("lambda and mu must be positive");
    }
    if (config.simulation_time <= 0.0) {
        throw std::invalid_argument("simulation_time must be positive");
    }

    // Initialize per-priority statistics vectors
    stats_.arrivals_per_priority.assign(config.num_priorities, 0ULL);
    stats_.drops_per_priority.assign(config.num_priorities, 0ULL);
    stats_.departures_per_priority.assign(config.num_priorities, 0ULL);
    stats_.total_waiting_time_per_priority.assign(config.num_priorities, 0.0);

    // Schedule the very first arrival
    schedule_next_arrival();

    // Optional: pre-allocate trace if enabled
    if (config.collect_queue_trace) {
        stats_.queue_length_trace.reserve(10000); // rough estimate
    }
}

void Simulator::set_queue(Queue* queue)
{
    if (queue == nullptr) {
        throw std::invalid_argument("queue pointer cannot be null");
    }
    queue_ = queue;
}

void Simulator::run()
{
    if (queue_ == nullptr) {
        throw std::runtime_error("No queue set before calling run()");
    }

    current_time_ = 0.0;

    // Reset all statistics
    stats_ = SimulationStats{};
    stats_.simulation_duration = config_.simulation_time;

    // Re-initialize per-priority vectors (in case reset wiped them)
    stats_.arrivals_per_priority.assign(config_.num_priorities, 0ULL);
    stats_.drops_per_priority.assign(config_.num_priorities, 0ULL);
    stats_.departures_per_priority.assign(config_.num_priorities, 0ULL);
    stats_.total_waiting_time_per_priority.assign(config_.num_priorities, 0.0);

    // Main event loop
    while (current_time_ < config_.simulation_time)
    {
        double next_event = get_next_event_time();

        if (next_event > config_.simulation_time) {
            break;
        }

        current_time_ = next_event;

        if (next_arrival_time_ <= current_time_)
        {
            process_arrival();
        }

        if (next_departure_time_ <= current_time_)
        {
            process_departure();
        }

        update_queue_length_stats();
    }

    // Final queue length update for remaining time
    update_queue_length_stats();
}

double Simulator::get_next_event_time() const
{
    return std::min(next_arrival_time_, next_departure_time_);
}

void Simulator::schedule_next_arrival()
{
    double interarrival = interarrival_dist_(rng_);
    next_arrival_time_ = current_time_ + interarrival;
}

void Simulator::process_arrival()
{
    stats_.total_arrivals++;

    double service_time = service_dist_(rng_);

    // Assign priority randomly
    std::uniform_real_distribution<double> prio_dist(0.0, 1.0);
    double r = prio_dist(rng_);

    int priority;
    if (r < 0.50) {
        priority = 0;           // high priority – 50%
    } else if (r < 0.85) {
        priority = 1;           // medium priority – 35%
    } else {
        priority = 2;           // low priority – 15%
    }

    Packet packet{
        current_time_,
        service_time,
        priority,
        next_packet_id_++
    };

    // Update per-priority arrival counter
    if (priority >= 0 && static_cast<size_t>(priority) < stats_.arrivals_per_priority.size()) {
        stats_.arrivals_per_priority[priority]++;
    }

    if (queue_->enqueue(packet))
    {
        if (!server_busy_)
        {
            start_service(packet);
        }
    }
    else
    {
        stats_.total_drops++;
        if (priority >= 0 && static_cast<size_t>(priority) < stats_.drops_per_priority.size()) {
            stats_.drops_per_priority[priority]++;
        }
    }

    schedule_next_arrival();
}

void Simulator::start_service(const Packet& packet)
{
    server_busy_ = true;
    current_serving_packet_ = packet;
    next_departure_time_ = current_time_ + packet.service_time;
}

void Simulator::process_departure()
{
    if (!server_busy_ || !current_serving_packet_.has_value())
    {
        return;
    }

    Packet pkt = *current_serving_packet_;

    double waiting_time = current_time_ - pkt.arrival_time - pkt.service_time;
    double system_time  = current_time_ - pkt.arrival_time;

    stats_.total_departures++;
    stats_.total_service_time += pkt.service_time;
    stats_.total_waiting_time += waiting_time;
    stats_.total_system_time  += system_time;

    // Update per-priority counters
    int prio = pkt.priority;
    if (prio >= 0 && static_cast<size_t>(prio) < stats_.departures_per_priority.size())
    {
        stats_.departures_per_priority[prio]++;
        stats_.total_waiting_time_per_priority[prio] += waiting_time;
    }

    current_serving_packet_.reset();
    server_busy_ = false;

    if (!queue_->is_empty())
    {
        Packet next_pkt = queue_->dequeue();
        start_service(next_pkt);
    }
    else
    {
        next_departure_time_ = std::numeric_limits<double>::infinity();
    }
}

void Simulator::update_queue_length_stats()
{
    size_t current_len = queue_->size();

    if (current_len > stats_.max_queue_length)
    {
        stats_.max_queue_length = current_len;
    }

    static double last_sample_time = 0.0;

    if (stats_.queue_length_sample_count > 0)
    {
        double time_interval = current_time_ - last_sample_time;
        stats_.sum_queue_length_samples += static_cast<double>(current_len) * time_interval;
    }

    stats_.queue_length_sample_count++;
    last_sample_time = current_time_;

    if (config_.collect_queue_trace)
    {
        stats_.queue_length_trace.emplace_back(current_time_, current_len);
    }
}

std::string Simulator::summary() const
{
    std::ostringstream oss;
    const auto& s = stats_;

    oss << std::fixed << std::setprecision(4);
    oss << "Simulation summary:\n";
    oss << "  Duration:          " << s.simulation_duration << " s\n";
    oss << "  Arrivals:          " << s.total_arrivals << "\n";
    oss << "  Departures:        " << s.total_departures << "\n";
    oss << "  Drops:             " << s.total_drops
        << " (" << s.drop_probability() * 100.0 << "%)\n";
    oss << "  Throughput:        " << s.avg_throughput() << " pkts/s\n";
    oss << "  Avg queue length:  " << s.avg_queue_length() << "\n";
    oss << "  Max queue length:  " << s.max_queue_length << "\n";
    oss << "  Avg waiting time:  " << s.avg_waiting_time() << " s\n";
    oss << "  Avg system time:   " << s.avg_system_time() << " s\n\n";

    // Per-priority table
    if (!s.arrivals_per_priority.empty())
    {
        oss << "Per-priority statistics:\n";
        oss << " Prio | Arrivals | Drops | Drop % | Departures | Avg wait (s)\n";
        oss << "-----|----------|-------|--------|------------|-------------\n";

        for (size_t p = 0; p < s.arrivals_per_priority.size(); ++p)
        {
            uint64_t arr = s.arrivals_per_priority[p];
            uint64_t drp = s.drops_per_priority[p];
            uint64_t dep = s.departures_per_priority[p];

            double drop_pct = arr > 0 ? 100.0 * drp / arr : 0.0;
            double avg_wait = dep > 0 ? s.total_waiting_time_per_priority[p] / dep : 0.0;

            oss << std::right << std::setw(4) << p << " | "
                << std::setw(8) << arr << " | "
                << std::setw(5) << drp << " | "
                << std::setw(6) << drop_pct << " | "
                << std::setw(10) << dep << " | "
                << std::setw(11) << avg_wait << "\n";
        }
    }

    return oss.str();
}