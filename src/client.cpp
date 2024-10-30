#include <zmq.hpp>
#include <string>
#include <iostream>

int main() {
    zmq::context_t context(1);
    zmq::socket_t clientSocket(context, zmq::socket_type::req);
    clientSocket.connect("tcp://localhost:5555"); // Connect to proxy

    std::string command = "create_list";
    zmq::message_t request(command.data(), command.size());
    clientSocket.send(request, zmq::send_flags::none);

    zmq::message_t reply;
    clientSocket.recv(reply, zmq::recv_flags::none);
    std::string receivedMsg(static_cast<char*>(reply.data()), reply.size());
    std::cout << "Server replied: " << receivedMsg << std::endl;

    return 0;
}