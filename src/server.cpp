#include <zmq.hpp>
#include <string>
#include <unordered_map>
#include <iostream>

std::unordered_map<std::string, std::string> shoppingLists;

std::string createShoppingList() {
    std::string id = "list_" + std::to_string(shoppingLists.size() + 1);
    shoppingLists[id] = "";
    return id;
}

int main() {
    zmq::context_t context(1);
    zmq::socket_t serverSocket(context, zmq::socket_type::rep);
    serverSocket.bind("tcp://*:5556"); // Connect to proxy via TCP port

    while (true) {
        zmq::message_t request;
        serverSocket.recv(request, zmq::recv_flags::none);
        std::string receivedMsg(static_cast<char*>(request.data()), request.size());

        std::string response;
        if (receivedMsg == "create_list") {
            std::string listId = createShoppingList();
            response = "Created list with ID: " + listId;
        } else if (shoppingLists.find(receivedMsg) != shoppingLists.end()) {
            response = "List found: " + receivedMsg;
        } else {
            response = "Error: Unknown command or list ID.";
        }

        zmq::message_t reply(response.c_str(), response.size());
        serverSocket.send(reply, zmq::send_flags::none);
    }
}