#include <iostream>
#include "../zmq/zmsg.hpp"
#include "../zmq/mdp.h"
#include "request_handler.cpp"
#include <nlohmann/json.hpp>
#include "majWorker.cpp"
#include <thread>

using json = nlohmann::json;

void run_worker(const std::string &broker, const std::string &service, bool verbose, const std::string &push_endpoint, const std::vector<std::string> &pull_endpoint) {
    mdwrk session(broker.c_str(), service.c_str(), "LIST_MANAGEMENT", verbose, push_endpoint, pull_endpoint);

    zmsg *reply = nullptr;
    while (true) {

        std::string message = "hello from " + push_endpoint;
        zmsg *msg_to_send = new zmsg(message.c_str());
        session.send_to_worker(msg_to_send);
        delete msg_to_send;

        zmsg *received_msg = session.recv_from_worker();
        if (received_msg) {
            std::cout << "Worker (" << push_endpoint << ") received:" << std::endl;
            received_msg->dump();
            delete received_msg;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        /*
        zmsg *request = session.recv(reply);
        if (!request) {
            break; // Worker interrupted
        }

        std::cout << "Worker got request:" << std::endl;
        request->dump();

        // Process the request (simple echo here)
        reply = request;
        */
        
    }
}

int main(int argc, char *argv[]) {
    bool verbose = (argc > 1 && strcmp(argv[1], "-v") == 0);

    std::vector<std::string> worker_endpoints = {
        "tcp://localhost:6000",
        "tcp://localhost:6001", 
        "tcp://localhost:6002"
    };

    std::cout << "Starting workers..." << std::endl;

    // Launch three threads for three independent workers
    std::thread worker1(run_worker, "", "", verbose, worker_endpoints[0], std::vector<std::string>{worker_endpoints[1], worker_endpoints[2]});
    std::thread worker2(run_worker, "", "", verbose, worker_endpoints[1], std::vector<std::string>{worker_endpoints[0], worker_endpoints[2]});
    std::thread worker3(run_worker, "", "", verbose, worker_endpoints[2], std::vector<std::string>{worker_endpoints[0], worker_endpoints[1]});

    worker1.join();
    worker2.join();
    //worker3.join();

    std::cout << "All workers finished." << std::endl;

    return 0;
}