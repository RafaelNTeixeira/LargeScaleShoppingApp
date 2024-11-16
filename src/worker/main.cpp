#include <iostream>
#include <zmq.hpp>
#include "request_handler.cpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main() {
    zmq::context_t context(1);
    zmq::socket_t brocker_socket(context, ZMQ_DEALER); // To receive tasks from broker
    brocker_socket.connect("tcp://localhost:5555");

    std::cout << "Server is running and listening on 5555..." << std::endl;

    initDatabase();

    zmq::pollitem_t items[] = {
        {static_cast<void*>(brocker_socket), 0, ZMQ_POLLIN, 0}
    };

    while (true) {
        zmq::poll(items, 1, std::chrono::milliseconds(-1));

        if (items[0].revents & ZMQ_POLLIN) {
            zmq::message_t request;
            auto rec = brocker_socket.recv(request, zmq::recv_flags::none);

            if (!rec) {
                std::cerr << "Worker failed to receive message." << std::endl;
                break;
            }

            std::string req_str(static_cast<char*>(request.data()), request.size());
            json request_json = json::parse(req_str);

            std::cout << "Received request: " << request_json.dump() << "\n";

            json response_json = handleRequest(request_json);

            // Send response back to requester
            std::string response_str = response_json.dump();
            zmq::message_t reply(response_str.size());
            memcpy(reply.data(), response_str.c_str(), response_str.size());
            brocker_socket.send(reply, zmq::send_flags::none);
        }
    }

    return 0;
}
