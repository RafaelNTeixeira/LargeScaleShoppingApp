#include <iostream>
#include "../zmq/zmsg.hpp"
#include "../zmq/mdp.h"
#include "request_handler.cpp"
#include <nlohmann/json.hpp>
#include "majWorker.cpp"
#include <thread>

using json = nlohmann::json;

void run_worker(const std::string &broker, const std::string &service, bool verbose) {
    mdwrk session(broker.c_str(), service.c_str(), "LIST_MANAGEMENT", verbose);

    zmsg *reply = nullptr;
    while (true) {
        zmsg *request = session.recv(reply);
        if (!request) {
            break; // Worker interrupted
        }

        std::cout << "Worker got request:" << std::endl;
        request->dump();

        // Process the request (simple echo here)
        reply = request;
    }
}

int main(int argc, char *argv[]) {
    bool verbose = (argc > 1 && strcmp(argv[1], "-v") == 0);

    std::cout << "Starting workers..." << std::endl;

    // Launch three threads for three independent workers
    std::thread worker1(run_worker, "tcp://localhost:5555", "tcp://*:5558", verbose);
    std::thread worker2(run_worker, "tcp://localhost:5555", "tcp://*:5559", verbose);
    std::thread worker3(run_worker, "tcp://localhost:5555", "tcp://*:5560", verbose);

    worker1.join();
    worker2.join();
    worker3.join();

    std::cout << "All workers finished." << std::endl;

    return 0;
}