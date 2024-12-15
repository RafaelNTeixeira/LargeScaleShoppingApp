#include <atomic>
#include <iostream>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <thread>
#include <unordered_set>
#include <vector>


#include "../zmq/mdp.h"
#include "../zmq/zmsg.hpp"
#include "majWorker.cpp"
#include "request_handler.cpp"


using json = nlohmann::json;

// std::vector<std::string> workers_ports;
// std::mutex ports_mutex;
// std::atomic<bool> is_running(true);
// ConsistentHashing* ch;

// json convert_ring_to_json(std::map<size_t, std::string> ring) {
//     json json_ring;
//     for (const auto& [key, value] : ring) {
//         json_ring[std::to_string(key)] = value;
//     }
//     return json_ring;
// }

// Method to connect workers PUSH sockets to other workers PULL sockets
// void monitor_worker_ports(std::unordered_set<std::string> &bound_ports, mdwrk &worker) {
//     while (is_running.load()) {
//         {
//             for (const auto &port : workers_ports) {
//             std::lock_guard<std::mutex> lock(ports_mutex); // Lock ports for thread safety
//                 std::cout << "Port: " << port << std::endl;
//                 if (bound_ports.find(port) == bound_ports.end()) { // If there are no new ports, skip
//                     std::string connect_address = "tcp://localhost:" + port; // Build connect address for PUSH socket
//                     std::cout << "connect_address: " << connect_address << std::endl;
//                     try {
//                         // Check if the worker's own push port is the same as this port
//                         size_t pos = worker.get_worker_pull_bind().find_last_of(':');
//                         std::string worker_pull_port = worker.get_worker_pull_bind().substr(pos + 1);
//                         std::cout << "worker_pull_port: " << worker_pull_port << std::endl;
//                         if (worker_pull_port != port) {
//                             worker.get_worker_push()->connect(connect_address.c_str());
//                             std::cout << "Successfully connected PULL worker to port: " << connect_address << std::endl;
//                             bound_ports.insert(port);
//                             std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // To assure other threads finish the other worker's connections
//                             std::cout << "Entering ring push" << std::endl;
//                             /*
//                             ConsistentHashing ring = worker.getConsitentHashing()->getRing();
//                             json json_ring = convert_ring_to_json(ring);
//                             if(ring.getNumberOfServers() == 0) {
//                                 std::cout << "Ring not initialised yet, goint to add myself" << std::endl;

//                             }
//                             worker.send_to_worker(k_mdpw_join_ring.data(), json_ring, NULL);
//                             */
//                             ch = worker.getConsistentHashing();
//                             json json_ring = convert_ring_to_json(ch->getRing());

//                             if (ch->getNumberOfServers() == 0) {
//                                 std::cout << "Ring not initialised yet, going to add myself" << std::endl;
//                                 ch->addServer(worker_pull_port);

//                                 json updated_ring = convert_ring_to_json(ch->getRing());
//                                 std::cout << "UPDATED RING: " << updated_ring << std::endl;

//                                 worker.send_to_worker(k_mdpw_broadcast_ring.data(), updated_ring, NULL);
//                                 std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//                             }
//                             else {
//                                 worker.send_to_worker(k_mdpw_join_ring.data(), json_ring, NULL);
//                             }
//                             std::cout << "Number of workers in the ring: " << ch->getNumberOfServers() << std::endl;
//                         }
//                     } catch (const zmq::error_t &e) {
//                         std::cerr << "Failed to connect PULL worker to port " << connect_address << ": " << e.what() << std::endl;
//                     }
//                 }
//             }
//         }
//         std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Check every 500ms
//     }
// }

void run_worker(const std::string &broker, const std::string &worker_pub_bind, const std::string &worker_pull_bind, const std::string &connect_to_worker, const std::string &database_path, bool verbose) {
    mdwrk session(broker.c_str(), worker_pub_bind.c_str(), worker_pull_bind.c_str(), connect_to_worker, "LIST_MANAGEMENT", database_path, verbose);

    zmsg *reply = nullptr;
    while (s_interrupted == 0) {
        zmsg *request = session.recv(reply);
        if (!request) {
            break;  // Worker interrupted
        }

        std::cout << "Worker got request:" << std::endl;
        request->dump();

        // Process the request (simple echo here)
        reply = request;
    }

    session.getDatabase().save(database_path);

    std::cout << "Worker shutting down." << std::endl;
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        std::cerr << "Usage: ./src/worker/worker <broker> <service> <worker_pull_port> <worker_connect_address> <database_path>" << std::endl;
        return 1;  // Error: incorrect number of arguments
    }

    bool verbose = (argc > 1 && strcmp(argv[1], "-v") == 0);

    std::cout << "Starting workers..." << std::endl;
    std::cout << argv[1] << argv[2] << argv[3] << argv[4] << std::endl;
    // Start workers with command-line arguments
    run_worker(argv[1], argv[2], argv[3], argv[4], argv[5], verbose);

    std::cout << "Worker finished." << std::endl;

    return 0;
}
