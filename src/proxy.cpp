#include <zmq.hpp>
#include <iostream>

int main() {
    zmq::context_t context(1);

    zmq::socket_t frontend(context, zmq::socket_type::router);
    frontend.bind("tcp://*:5555"); // Client connections

    zmq::socket_t backend(context, zmq::socket_type::dealer);
    backend.connect("tcp://localhost:5556"); // Connect to server

    zmq::proxy(frontend, backend);

    return 0;
}