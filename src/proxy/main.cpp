#include <iostream>
#include <zmq.hpp>
//#include "request_handler.cpp"

int main() {
    zmq::context_t context(1);
    zmq::socket_t router_socket(context, ZMQ_ROUTER); 
    router_socket.bind("tcp://*:5556"); // Establish connection with the client

    zmq::socket_t dealer_socket(context, ZMQ_DEALER);
    dealer_socket.bind("tcp://*:5557"); // Establish connection with the server

    std::cout << "Proxy is running..." << std::endl;

    while (true) {
        zmq::message_t identity;
        router_socket.recv(identity, zmq::recv_flags::none);

        zmq::message_t request;
        router_socket.recv(request, zmq::recv_flags::none); // Receive request from client

        std::string req_str(static_cast<char*>(request.data()), request.size());
        std::cout << "Received request: " << req_str << std::endl;

        zmq::socket_t server_socket(context, ZMQ_DEALER);
        server_socket.connect("tcp://localhost:5555"); // Connect to server

        server_socket.send(request, zmq::send_flags::none); // Send the client request to the server

        zmq::message_t reply;
        server_socket.recv(reply, zmq::recv_flags::none); // Wait for server reply

        router_socket.send(identity, zmq::send_flags::sndmore); // sndmore = To be sent together with a following message
        router_socket.send(reply, zmq::send_flags::none);
    }

    return 0;
}