#include <iostream>
#include "../zmq/zmsg.hpp"
#include "../zmq/mdp.h"
#include "request_handler.cpp"
#include <nlohmann/json.hpp>
#include "majWorker.cpp"

using json = nlohmann::json;

int main(int argc, char *argv []) {
    int verbose = (argc > 1 && strcmp (argv [1], "-v") == 0);
    mdwrk session ("tcp://localhost:5555", "tcp://*:5558", "LIST_MANAGEMENT", verbose);

    std::cout << "Worker is running..." << std::endl;

    //initDatabase();

    zmsg *reply = 0;
    while (true) {
        zmsg *request = session.recv (reply);
        std::cout << "Worker got request: " << std::endl;
        request->dump();
        if (request == 0) {
            break; //  Worker was interrupted
        }
        reply = request; //  Echo is complex... :-)
    }
    return 0;
}
