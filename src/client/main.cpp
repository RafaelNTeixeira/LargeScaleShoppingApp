#include <iostream>
#include <zmq.hpp>
#include "ui.cpp"
#include <nlohmann/json.hpp>
#include <uuid/uuid.h>
using json = nlohmann::json;

std::string generateUUID() {
    uuid_t uuid;
    uuid_generate_random(uuid);

    char uuid_str[37];
    uuid_unparse(uuid, uuid_str);

    return std::string(uuid_str);
}

int main() {
    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_DEALER);
    socket.connect("tcp://localhost:5556");

    std::cout << "Client is running..." << std::endl;

    json request_json;

    int choice;
    while (true) {
        displayMenu();
        std::cin >> choice;

        switch (choice) {
            case 1: {
                std::string base_url = "https://myshoppinglistapp.com/list/";
                std::string list_id = generateUUID();
                std::string full_url = base_url + list_id;
                request_json["command"] = "CREATE_LIST";
                request_json["parameters"] = {{"list_id", full_url}, {"name", "My List"}};
                break;
            }
            case 2: {
                request_json["command"] = "GET_LIST";
                break;
            }
            case 3: {
                return 0;
            }
            default: {
                std::cout << "Invalid choice. Try again.\n\n";
                continue;
            }
        }

        std::string request = request_json.dump();;
        std::cout << request << std::endl;

        zmq::message_t msg(request.size());
        memcpy(msg.data(), request.c_str(), request.size());
        socket.send(msg, zmq::send_flags::none); // Send message to proxy

        zmq::message_t reply;
        socket.recv(reply, zmq::recv_flags::none); // Receive message from proxy (sent by server)

        std::string response(static_cast<char*>(reply.data()), reply.size());
        std::cout << "Response from server: " << response << std::endl;
    }

    return 0;
}