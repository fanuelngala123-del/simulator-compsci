#include "Simulator.h"
#include "FIFOQueue.h"
#include "PriorityQueue.h"
#include "httplib.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>
#include <stdexcept>

// Shared CSS to keep both pages looking consistent
const std::string SHARED_CSS = R"(
    <style>
        :root { --primary: #2563eb; --bg: #f8fafc; --card: #ffffff; --text: #1e293b; --accent: #3b82f6; }
        body { font-family: 'Inter', system-ui, -apple-system, sans-serif; background: var(--bg); color: var(--text); line-height: 1.5; margin: 0; padding: 20px; }
        .container { max-width: 1000px; margin: 0 auto; }
        .card { background: var(--card); padding: 24px; border-radius: 12px; box-shadow: 0 4px 6px -1px rgba(0,0,0,0.1); margin-bottom: 24px; }
        h1 { color: #0f172a; font-size: 1.875rem; margin-bottom: 0.5rem; }
        .subtitle { color: #64748b; margin-bottom: 2rem; }
        
        /* Form Layout */
        .form-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; }
        .form-group { display: flex; flex-direction: column; margin-bottom: 15px; }
        label { font-weight: 600; font-size: 0.875rem; margin-bottom: 6px; color: #475569; }
        input { padding: 10px; border: 1px solid #cbd5e1; border-radius: 6px; font-size: 1rem; }
        input:focus { outline: 2px solid var(--accent); border-color: transparent; }
        button { background: var(--primary); color: white; padding: 12px 24px; border: none; border-radius: 6px; font-weight: 600; cursor: pointer; transition: background 0.2s; margin-top: 10px; }
        button:hover { background: #1d4ed8; }

        /* Results Display */
        .results-container { display: grid; grid-template-columns: repeat(auto-fit, minmax(400px, 1fr)); gap: 24px; }
        table { width: 100%; border-collapse: collapse; margin: 10px 0; font-size: 0.95rem; }
        th { text-align: left; color: #64748b; font-weight: 500; padding: 8px 0; border-bottom: 1px solid #f1f5f9; }
        td { text-align: right; font-weight: 600; font-family: 'JetBrains Mono', monospace; color: var(--primary); padding: 8px 0; border-bottom: 1px solid #f1f5f9; }
        .trace-log { background: #0f172a; color: #38bdf8; padding: 15px; border-radius: 8px; font-family: monospace; font-size: 0.85rem; overflow-x: auto; white-space: pre-wrap; }
        .back-link { display: inline-block; margin-bottom: 20px; text-decoration: none; color: var(--primary); font-weight: 500; }
        .error { color: #dc2626; background: #fee2e2; padding: 15px; border-radius: 8px; font-weight: 600; }
    </style>
)";

std::string escape_html(const std::string& s) {
    std::string result;
    result.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '&':  result += "&amp;"; break;
            case '<':  result += "&lt;"; break;
            case '>':  result += "&gt;"; break;
            case '"':  result += "&quot;"; break;
            case '\'': result += "&#39;"; break;
            default:   result += c;
        }
    }
    return result;
}

// Helper to turn raw summary text into a clean HTML table
std::string format_summary_table(const std::string& summary) {
    std::ostringstream oss;
    oss << "<table>";
    std::stringstream ss(summary);
    std::string line;
    while (std::getline(ss, line)) {
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            oss << "<tr><th>" << line.substr(0, colon) << "</th><td>" 
                << line.substr(colon + 1) << "</td></tr>";
        }
    }
    oss << "</table>";
    return oss.str();
}

std::string build_form() {
    std::ostringstream oss;
    oss << "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'>"
        << "<title>Packet Queue Simulator</title>" << SHARED_CSS << "</head><body>"
        << "<div class='container'><h1>Packet Queue Simulator</h1>"
        << "<p class='subtitle'>Analyze network performance using M/M/1/K modeling</p>"
        << "<div class='card'><form method='POST' action='/simulate' class='form-grid'>"
        << "<div class='form-group'><label>Arrival rate λ (pkts/s)</label><input type='number' step='0.1' name='lambda' value='4.0' required></div>"
        << "<div class='form-group'><label>Service rate μ (pkts/s)</label><input type='number' step='0.1' name='mu' value='5.0' required></div>"
        << "<div class='form-group'><label>Buffer size (packets)</label><input type='number' name='buffer' value='20' required></div>"
        << "<div class='form-group'><label>Simulation time (seconds)</label><input type='number' name='time' value='1000' required></div>"
        << "<div class='form-group'><label>Number of priorities</label><input type='number' name='priorities' value='3' max='8' required></div>"
        << "<div class='form-group'><label>Priority probabilities</label><input type='text' name='probs' value='0.5,0.35,0.15' required></div>"
        << "<div class='form-group' style='grid-column: 1 / -1;'><label style='display:flex; align-items:center;'>"
        << "<input type='checkbox' name='trace' value='1' style='width:auto; margin-right:10px;'> Collect queue length trace</label></div>"
        << "<button type='submit' style='grid-column: 1 / -1;'>Run Analysis</button></form></div></div></body></html>";
    return oss.str();
}

std::string get_param(const httplib::Params& params, const std::string& key, const std::string& default_val = "") {
    auto it = params.find(key);
    return (it != params.end()) ? it->second : default_val;
}

std::string run_simulation(const httplib::Params& params) {
    SimulationConfig cfg;
    try {
        cfg.lambda = std::stod(get_param(params, "lambda", "4.0"));
        cfg.mu = std::stod(get_param(params, "mu", "5.0"));
        cfg.buffer_size = std::stoul(get_param(params, "buffer", "20"));
        cfg.num_priorities = std::stoi(get_param(params, "priorities", "3"));
        cfg.simulation_time = std::stod(get_param(params, "time", "1000.0"));

        std::string probs_str = get_param(params, "probs", "0.5,0.35,0.15");
        std::vector<double> probs;
        std::stringstream ss(probs_str);
        std::string token;
        while (std::getline(ss, token, ',')) probs.push_back(std::stod(token));
        
        if (probs.size() != static_cast<size_t>(cfg.num_priorities)) 
            throw std::runtime_error("Priority count mismatch");
        cfg.priority_probs = std::move(probs);
        cfg.collect_queue_trace = params.find("trace") != params.end();
    } catch (const std::exception& e) {
        return "<div class='error'>Configuration Error: " + std::string(e.what()) + "</div>";
    }

    // Run FIFO
    FIFOQueue fifo(cfg.buffer_size);
    Simulator fifo_sim(cfg);
    fifo_sim.set_queue(&fifo);
    fifo_sim.run();

    // Run Priority Queue
    PriorityQueue pq(cfg.buffer_size, cfg.num_priorities);
    Simulator pq_sim(cfg);
    pq_sim.set_queue(&pq);
    pq_sim.run();

    std::ostringstream res;
    res << "<div class='results-container'>"
        << "<div class='card'><h2>FIFO (First-In, First-Out)</h2>" << format_summary_table(fifo_sim.summary()) << "</div>"
        << "<div class='card'><h2>Priority Queueing</h2>" << format_summary_table(pq_sim.summary()) << "</div>"
        << "</div>";

    if (cfg.collect_queue_trace) {
        res << "<div class='card'><h3>Priority Queue Length Trace (Tail)</h3><div class='trace-log'>";
        const auto& trace = pq_sim.get_stats().queue_length_trace;
        size_t start = (trace.size() > 50) ? trace.size() - 50 : 0;
        for (size_t i = start; i < trace.size(); ++i) {
            res << "t=" << std::fixed << std::setprecision(2) << trace[i].first << " &rarr; " << (int)trace[i].second << " pkts | ";
        }
        res << "</div></div>";
    }
    return res.str();
}

int main() {
    httplib::Server svr;

    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(build_form(), "text/html");
    });

    svr.Post("/simulate", [](const httplib::Request& req, httplib::Response& res) {
        std::string content = "<!DOCTYPE html><html><head><meta charset='UTF-8'>" + SHARED_CSS 
            + "</head><body><div class='container'><a href='/' class='back-link'>&larr; Back to Parameters</a>"
            + "<h1>Simulation Analysis</h1>" + run_simulation(req.params) + "</div></body></html>";
        res.set_content(content, "text/html");
    });

    std::cout << "Server running at http://localhost:8080\n";
    svr.listen("0.0.0.0", 8080);
    return 0;
}