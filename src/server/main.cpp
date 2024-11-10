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

    while (true) {
        zmq::message_t request;
        socket.recv(request, zmq::recv_flags::none); // Receive request from the proxy

        std::string req_str(static_cast<char*>(request.data()), request.size());
        std::cout << "Received request: " << req_str << std::endl;

        json request_json = json::parse(req_str);

        std::cout << "Response from client: " << request_json << std::endl;

        std::string command = request_json["command"];
        json response_json;

        if (command == "CREATE_LIST") {
            std::string list_id = request_json["parameters"]["list_id"];
            response_json["status"] = "success";
            response_json["message"] = "List created with ID " + list_id;
        } 
        else if (command == "GET_LIST") {
            response_json["status"] = "success";
            response_json["message"] = "List retrieved";
        }
        else {
            response_json["status"] = "error";
            response_json["message"] = "Unknown command";
        }

        std::string response_str = response_json.dump();
        zmq::message_t reply(response_str.size());
        memcpy(reply.data(), response_str.c_str(), response_str.size());

        socket.send(reply, zmq::send_flags::none); // Send the response back to the client via proxy
    }

    return 0;
}
