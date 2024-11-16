#include <iostream>
#include <deque>
#include <unordered_set>
#include <zmq.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main() {
    zmq::context_t context(1);

    zmq::socket_t worker_router(context, ZMQ_ROUTER);
    worker_router.bind("tcp://*:5555");

    zmq::socket_t client_router(context, ZMQ_ROUTER);
    client_router.bind("tcp://*:5556");

    zmq::socket_t pull_socket(context, ZMQ_PULL);
    pull_socket.bind("tcp://*:5557"); // For proxy to receive list updates

    zmq::socket_t xpub_socket(context, ZMQ_XPUB);
    xpub_socket.bind("tcp://*:5558"); // For proxy to transmit to clients list updates

    std::cout << "Proxy is running..." << std::endl;

    std::deque<std::vector<zmq::message_t>> client_queue;
    std::unordered_set<std::string> available_workers;

    zmq::pollitem_t items[] = {
        {static_cast<void*>(client_router), 0, ZMQ_POLLIN, 0},  // Client requests
        {static_cast<void*>(worker_router), 0, ZMQ_POLLIN, 0},  // Worker pings or responses
        {static_cast<void*>(pull_socket), 0, ZMQ_POLLIN, 0},    // Server updates
    };

    while (true) {
        zmq::poll(items, 3, std::chrono::milliseconds(-1));

        std::cout << "Entered While" << std::endl;

        // Handle client list requests
        if (items[0].revents & ZMQ_POLLIN) {
            std::cout << "Entered client requests" << std::endl;
            std::vector<zmq::message_t> client_request;

            // (identity + request)
            while (true) {
                zmq::message_t part;
                auto rec = client_router.recv(part, zmq::recv_flags::none);
                client_request.push_back(std::move(part));
                if (!client_request.back().more()) break;
            }

            // Queue the client request for an available worker
            client_queue.push_back(std::move(client_request));
        }

        // Handle worker requests (ping for tasks)
        if (items[1].revents & ZMQ_POLLIN) {
            std::cout << "Entered worker requests" << std::endl;
            std::vector<zmq::message_t> worker_message;
            zmq::message_t worker_id;
            auto rec = worker_router.recv(worker_id, zmq::recv_flags::none);
            available_workers.insert(worker_id.to_string());

            while (true) {
                zmq::message_t part;
                auto rec = worker_router.recv(part, zmq::recv_flags::none);
                worker_message.push_back(std::move(part));
                if (!worker_message.back().more()) break;
            }

            if (worker_message.size() == 1) {
                if (!client_queue.empty()) {
                    auto client_request = std::move(client_queue.front());
                    client_queue.pop_front();

                    // Send client request to worker
                    worker_router.send(worker_id, zmq::send_flags::sndmore);
                    for (size_t i = 0; i < client_request.size(); ++i) {
                        worker_router.send(client_request[i], (i < client_request.size() - 1) ? zmq::send_flags::sndmore : zmq::send_flags::none);
                    }
                    available_workers.erase(worker_id.to_string()); // Worker is now busy
                }
            } else {
                // Worker is responding with a client reply
                zmq::message_t client_id = std::move(worker_message[0]);
                client_router.send(client_id, zmq::send_flags::sndmore);
                for (size_t i = 1; i < worker_message.size(); ++i) {
                    client_router.send(worker_message[i], (i < worker_message.size() - 1) ? zmq::send_flags::sndmore : zmq::send_flags::none);
                }
            }
        }

        // Handle updates client list updates
        if (items[2].revents & ZMQ_POLLIN) {
            std::cout << "Entered client list updates" << std::endl;
            zmq::message_t update;
            auto rec = pull_socket.recv(update, zmq::recv_flags::none);

            // Publish the update to clients
            xpub_socket.send(update, zmq::send_flags::none);
            std::cout << "Entered client list updates" << std::endl;
        }
    }

    return 0;
}