#include <iostream>
#include <zmq.hpp>
#include "ui.cpp"

int main() {
    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_DEALER);
    socket.connect("tcp://localhost:5556");

    std::cout << "Client is running..." << std::endl;

    int choice;
    while (true) {
        displayMenu();
        std::cin >> choice;

        std::string request;
        if (choice == 1) {
            std::string list_name;
            std::cout << "Enter the name of the shopping list: ";
            std::cin >> list_name;

            request = "CREATE_LIST:" + list_name;
        } else if (choice == 2) {
            std::string url;
            std::cout << "Enter the URL of the shopping list: ";
            std::cin >> url;

            request = "GET_LIST:" + url;
        } else if (choice == 3) {
            break;
        } else {
            std::cout << "Invalid choice. Try again.\n\n";
            continue;
        }

        zmq::message_t msg(request.size());
        memcpy(msg.data(), request.c_str(), request.size());
        socket.send(msg, zmq::send_flags::none);

        zmq::message_t reply;
        socket.recv(reply, zmq::recv_flags::none);

        std::string response(static_cast<char*>(reply.data()), reply.size());
        std::cout << "Response from server: " << response << std::endl;
    }

    return 0;
}
