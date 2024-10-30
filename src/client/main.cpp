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
        switch (choice) {
            case 1:
                request = "CREATE_LIST";
                break;
            case 2:
                request = "GET_LIST";
                break;
            case 3:
                return 0;
            default:
                std::cout << "Invalid choice. Try again.\n\n";
                continue;
        }

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