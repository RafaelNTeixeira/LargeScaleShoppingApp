#include <iostream>
#include <vector>
#include <thread>
#include <unordered_set>
#include <mutex>
#include <atomic>
#include <memory>
#include "../zmq/zmsg.hpp"
#include "../zmq/mdp.h"
#include "request_handler.cpp"
#include <nlohmann/json.hpp>
#include "majWorker.cpp"

using json = nlohmann::json;

std::vector<std::string> workers_ports;
std::mutex ports_mutex;
std::atomic<bool> is_running(true);

json convert_ring_to_json(ConsistentHashing ring_) {
    std::map<size_t, std::string> ring = ring_.getRing();
    json json_ring;
    for (const auto& [key, value] : ring) {
        json_ring[std::to_string(key)] = value;
    }
    return json_ring;
}

// Method to connect workers PULL sockets to other workers PUSH sockets
void monitor_worker_ports(zmq::context_t &context, std::unordered_set<std::string> &bound_ports, mdwrk &worker) {
    std::cout << "Entered monitor_worker_ports" << std::endl;
    while (is_running.load()) {
        {
            std::lock_guard<std::mutex> lock(ports_mutex); // Lock ports for thread safety
            std::cout << "1-monitor" << std::endl;
            for (const auto &port : workers_ports) {
                std::cout << "2-monitor" << std::endl;
                std::cout << "Port: " << port << std::endl;
                if (bound_ports.find(port) == bound_ports.end()) { // If there are no new ports, skip
                    std::string connect_address = "tcp://localhost:" + port; // Build connect address for pull socket
                    std::cout << "connect_address: " << connect_address << std::endl;
                    try {
                        // Check if the worker's own push port is the same as this port
                        size_t pos = worker.get_worker_push_bind().find_last_of(':');
                        std::string worker_push_port = worker.get_worker_push_bind().substr(pos + 1);
                        std::cout << "worker_push_port: " << worker_push_port << std::endl;
                        if (worker_push_port != port) {
                            worker.get_worker_pull()->connect(connect_address.c_str());
                            bound_ports.insert(port);
                            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                            std::cout << "Entering ring push" << std::endl;
                            ConsistentHashing ring = worker.getRing();
                            json json_ring = convert_ring_to_json(ring);
                            worker.send_to_worker(k_mdpw_join_ring.data(), json_ring, NULL);
                            std::cout << "Successfully connected PULL worker to port: " << connect_address << std::endl;
                        }
                    } catch (const zmq::error_t &e) {
                        std::cerr << "Failed to connect PULL worker to port " << connect_address << ": " << e.what() << std::endl;
                    }
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Check every 500ms
    }
}

void run_worker(const std::string &broker, const std::string &service, const std::string &worker_push_port, bool verbose) {
    {
        std::lock_guard<std::mutex> lock(ports_mutex);
        workers_ports.push_back(worker_push_port);
    }

    std::string worker_push_bind = "tcp://*:" + worker_push_port; // Create bind address for PUSH socket
    mdwrk session(broker.c_str(), service.c_str(), worker_push_bind.c_str(), "LIST_MANAGEMENT", verbose);

    // To track connected ports
    std::unordered_set<std::string> bound_ports;
        zmq::context_t context(1);

    // Thread to monitor new workers connections
    std::thread monitor_thread(monitor_worker_ports, std::ref(context), std::ref(bound_ports), std::ref(session));

    zmsg *reply = nullptr;
    while (is_running.load()) {
        zmsg *request = session.recv(reply);
        if (!request) {
            break; // Worker interrupted
        }

        std::cout << "Worker got request:" << std::endl;
        request->dump();

        // Process the request (simple echo here)
        reply = request;
    }

    is_running.store(false);
    monitor_thread.join();

    std::cout << "Worker shutting down." << std::endl;
}

int main(int argc, char *argv[]) {
    bool verbose = (argc > 1 && strcmp(argv[1], "-v") == 0);

    std::cout << "Starting workers..." << std::endl;

    // Launch three threads for three independent workers
    std::thread worker1(run_worker, "tcp://localhost:5555", "tcp://*:5558", "5601", verbose);
    std::thread worker2(run_worker, "tcp://localhost:5555", "tcp://*:5559", "5602", verbose);
    //std::thread worker3(run_worker, "tcp://localhost:5555", "tcp://*:5560", "5603", verbose);

    worker1.join();
    worker2.join();
    //worker3.join();

    std::cout << "All workers finished." << std::endl;

    return 0;
}