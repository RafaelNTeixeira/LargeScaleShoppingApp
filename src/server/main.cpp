#include <iostream>
#include <zmq.hpp>
#include "request_handler.cpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main() {
    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_DEALER);
    socket.bind("tcp://*:5555");

    std::cout << "Server is running and listening on 5555..." << std::endl;

    initDatabase(); 

    while (true) {
        zmq::message_t request;
        socket.recv(request, zmq::recv_flags::none);

        std::string req_str(static_cast<char*>(request.data()), request.size());

        json request_json = json::parse(req_str);
        std::cout << "Response from client: " << request_json << std::endl;

        json response_json = handleRequest(request_json);

        std::string response_str = response_json.dump();
        zmq::message_t reply(response_str.size());
        memcpy(reply.data(), response_str.c_str(), response_str.size());

        socket.send(reply, zmq::send_flags::none); // Send the response back to the client via proxy
        
    }

    return 0;
}
