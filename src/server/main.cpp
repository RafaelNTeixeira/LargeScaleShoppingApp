#include <iostream>
#include <zmq.hpp>
#include "api.cpp"
#include "request_handler.cpp"

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

        // response = handleRequest(req_str); 

        if (req_str == "GET_LIST") {
            ShoppingList list = getShoppingList("list1"); // TEMP. In the future, change to the list requested by the user
            std::string response = "Retrieved List Name: " + list.name;
            zmq::message_t reply(response.size());
            memcpy(reply.data(), response.c_str(), response.size());
            socket.send(reply, zmq::send_flags::none); // Reply to the proxy
        } 
        else if (req_str == "CREATE_LIST") {
            // TEMP. In the future, create the list according to what user manually adds
            ShoppingList newList = {"list1", "user123", "Grocery List 1", std::time(nullptr), std::time(nullptr)};
            createShoppingList(newList);
            std::string response = "Created list: " + newList.name;
            zmq::message_t reply(response.size());
            memcpy(reply.data(), response.c_str(), response.size());
            socket.send(reply, zmq::send_flags::none); // Reply to the proxy
        }
    }

    return 0;
}
