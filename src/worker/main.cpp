#include <iostream>
#include "../zmq/zmsg.hpp"
#include "../zmq/mdp.h"
#include "request_handler.cpp"
#include <nlohmann/json.hpp>
#include "majWorker.cpp"

using json = nlohmann::json;

int main(int argc, char *argv []) {
    int verbose = (argc > 1 && strcmp (argv [1], "-v") == 0);
    mdwrk session ("tcp://localhost:5555", "GET_LIST", verbose);

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

    // while (true) {
    //     zmq::poll(items, 1, std::chrono::milliseconds(-1));

    //     if (items[0].revents & ZMQ_POLLIN) {
    //         zmq::message_t request;
    //         auto rec = brocker_socket.recv(request, zmq::recv_flags::none);

    //         if (!rec) {
    //             std::cerr << "Worker failed to receive message." << std::endl;
    //             break;
    //         }

    //         std::string req_str(static_cast<char*>(request.data()), request.size());
    //         json request_json = json::parse(req_str);

    //         std::cout << "Received request: " << request_json.dump() << "\n";

    //         json response_json = handleRequest(request_json);

    //         // Send response back to requester
    //         std::string response_str = response_json.dump();
    //         zmq::message_t reply(response_str.size());
    //         memcpy(reply.data(), response_str.c_str(), response_str.size());
    //         brocker_socket.send(reply, zmq::send_flags::none);
    //     }
    // }

    return 0;
}
