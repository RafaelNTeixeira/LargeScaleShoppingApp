#ifndef __MDCLIAPI_HPP_INCLUDED__
#define __MDCLIAPI_HPP_INCLUDED__

#include "../zmq/mdp.h"
#include "../zmq/zmsg.hpp"
#include <zmq.hpp>
#include <string>
#include <vector>
#include <iostream>
#include <uuid/uuid.h>
#include <algorithm>

//  Structure of our class
//  We access these properties only via class methods
// static constexpr uint32_t n_heartbeat_expiry =  2500;    //  msecs

class mdcli {
   public:
    //  ---------------------------------------------------------------------
    //  Constructor
    mdcli(std::string broker, std::string broker_heartbeat,int verbose) : m_broker(broker), m_broker_heartbeat(broker_heartbeat), m_verbose(verbose) {
        s_version_assert(4, 0);
        m_context = new zmq::context_t(1);
        // s_catch_signals ();
        connect_to_broker();
        connect_sub_socket();
    }

    //  ---------------------------------------------------------------------
    //  Destructor
    virtual ~mdcli() {
        delete m_sub_socket;
        delete m_client;
        delete m_client_heartbeat;
        delete m_context;
    }

    static std::string generateUUID() {
        uuid_t uuid;
        uuid_generate_random(uuid);

        char uuid_str[37];
        uuid_unparse(uuid, uuid_str);

        return std::string(uuid_str);
    }

    //  ---------------------------------------------------------------------
    //  Connect or reconnect to broker
    void connect_to_broker() {
        if (m_client) {
            delete m_client;
        }
        if (m_client_heartbeat) {
            delete m_client_heartbeat;
        }
        m_client = new zmq::socket_t(*m_context, ZMQ_DEALER);
        m_client_heartbeat = new zmq::socket_t(*m_context, ZMQ_DEALER);

        int linger = 0;  // Discard unsent messages immediately
        m_client->set(zmq::sockopt::linger, linger);
        m_client_heartbeat->set(zmq::sockopt::linger, linger);

        std::string client_id = generateUUID();
        m_client->set(zmq::sockopt::routing_id, client_id);
        m_client->connect(m_broker.c_str());

        m_client_heartbeat->set(zmq::sockopt::routing_id, client_id);
        m_client_heartbeat->connect(m_broker_heartbeat.c_str());

        m_heartbeat_at = s_clock() + m_heartbeat;
        if (m_verbose)
            s_console("I: connecting to broker at %s...", m_broker.c_str());
            s_console("I: connecting to broker HEARTBEAT at %s...", m_broker_heartbeat.c_str());
    }

    // Initialize and connect SUB socket
    void connect_sub_socket() {
        m_sub_socket = new zmq::socket_t(*m_context, ZMQ_SUB);
        m_sub_socket->connect("tcp://localhost:5557");

        // SUBSCRIBING TO ALL CHANGES MADE TO THE LISTS THAT A CLIENT HAS ACCESS TO
        for (std::string &client_url : client_url_list) {
            m_sub_socket->set(zmq::sockopt::subscribe, client_url);
        }
    }

    void subscribe_to_list(std::string list_url) {
        m_sub_socket->set(zmq::sockopt::subscribe, list_url);
        if (m_verbose) {
            std::cout << "Subscribed to list: " << list_url << std::endl;
        }
        client_url_list.push_back(list_url);
    }

    void unsubscribe_from_list(std::string list_url) {
        m_sub_socket->set(zmq::sockopt::unsubscribe, list_url);
        client_url_list.erase(std::remove(client_url_list.begin(), client_url_list.end(), list_url), client_url_list.end());
    }

    // Receive update notification via SUB socket
    std::vector<std::string> receive_updates() {
        zmq::message_t message;
        std::vector<std::string> results;

        // Loop to process all available messages
        while (results.empty() && m_sub_socket->recv(message, zmq::recv_flags::dontwait)) {

            // PART 0: URL_LIST
            // PART 1: SHOPPING_LIST
            zmsg *msg = new zmsg();

            char *url = static_cast<char *>(message.data());
            url[message.size()] = '\0';
            msg->push_back(url);  // Add the first part

            // Continue receiving other parts if multipart
            while (m_sub_socket->recv(message, zmq::recv_flags::dontwait)) {
                char *part = static_cast<char *>(message.data());
                part[message.size()] = '\0';
                msg->push_back(part);
            }

            // Process the zmsg
            // NEEDS TO BE UPDATED TO HANDLE THE SHOPPING LIST
            if (msg->parts() > 0) {
                if (m_verbose) {
                    std::cout << "Received active update through SUB socket:" << std::endl;
                    msg->dump();
                }

                std::string url_list = ((char*)msg->pop_front().c_str());
                results.push_back(url_list);  // Add the URL list to results
                std::string shopping_list;

                if (msg->parts() > 0) {
                    shopping_list = ((char*)msg->pop_front().c_str());

                    results.push_back(shopping_list);  // Add the shopping list to results
                } else {
                    std::cerr << "Expected a shopping list part, but none was found!" << std::endl;
                }
            }

            delete msg;
        }

        return results;
    }

    //  ---------------------------------------------------------------------
    //  Set request timeout
    void set_timeout(int timeout) {
        m_timeout = timeout;
    }

    //  ---------------------------------------------------------------------
    //  Set heartbeat delay
    void set_heartbeat(int heartbeat) {
        m_heartbeat = heartbeat;
    }

    //  ---------------------------------------------------------------------
    //  Get cloud_mode
    bool get_cloud_mode() {
        return cloud_mode;
    }

    //  ---------------------------------------------------------------------
    //  Send request to broker
    //  Takes ownership of request message and destroys it when sent.
    int send(std::string service, std::string sub_request, zmsg *&request_p) {
        assert(request_p);
        zmsg *request = request_p;

        // if (m_verbose) {
        //     s_console("I: send request to '%s' service:", service.c_str());
        //     request->dump();
        // }

        //  Prefix request with protocol frames
        //  Frame 0: empty (REQ emulation)
        //  Frame 1: "MDPCxy" (six bytes, MDP/Client x.y)
        //  Frame 2: Service Name (printable string) (HEARTBEAT OR LIST_MANAGEMENT)
        //  Frame 3: Request Name (printable string)

        request->push_front(sub_request.c_str());
        request->push_front(service.c_str());
        request->push_front(k_mdp_client.data());
        request->push_front("");

        if (service != "HEARTBEAT"){
            // request->dump();
        }

        request->send(*m_client);
        std::string endpoint = m_client->get(zmq::sockopt::last_endpoint);
        // std::cout << "Sent request through DEALER socket: " << endpoint << std::endl;

        return 0;
    }

    //  ---------------------------------------------------------------------
    //  Returns the reply message or NULL if there was no reply. Does not
    //  attempt to recover from a broker failure, this is not possible
    //  without storing all unanswered requests and resending them all...
    zmsg *recv() {
        //  Poll socket for a reply, with timeout
        zmq::pollitem_t items[] = {
            {*m_client, 0, ZMQ_POLLIN, 0}};
        zmq::poll(items, 1, std::chrono::milliseconds(m_timeout));

        //  If we got a reply, process it
        if (items[0].revents & ZMQ_POLLIN) {
            zmsg *msg = new zmsg(*m_client);
            // std::cout << "Client got reply:" << std::endl;
            // msg->dump();

            // if (m_verbose) {
            //     s_console("I: received reply:");
            //     msg->dump();
            // }

            if (msg->parts() < 4) {
                std::cout << "Received invalid message from broker" << std::endl;
                msg->dump();
            }
            assert(msg->parts() >= 4);

            assert(msg->pop_front().length() == 0);  // empty message

            ustring header = msg->pop_front();
            // std::cout << "HEADER: " << header.c_str() << std::endl;
            if (header.compare((unsigned char *)k_mdp_client.data()) == 0) {
                ustring service = msg->pop_front();
                assert(service.compare((unsigned char *)service.c_str()) == 0);
            }

            return msg;  // Success
        }

        if (s_clock() >= m_heartbeat_at) {
            zmsg *message = new zmsg();
            int res = send("HEARTBEAT", "", message);
            // std::cout << "Sending HEARTBEAT to Broker" << std::endl;
            m_heartbeat_at += m_heartbeat;
            // std::cout << "cloud_mode: " << cloud_mode << std::endl;
        }

        if (s_clock() >= n_heartbeat_expiry) {
            // std::cout << "HEARTBEAT EXPIRED" << std::endl;
            cloud_mode = false;
            // std::cout << "cloud_mode: " << cloud_mode << std::endl;
        }

        if (s_interrupted) {
            // std::cout << "W: interrupt received, killing client..." << std::endl;
        } else if (m_verbose) {
            // s_console("W: permanent error, abandoning request");
        }

        return 0;
    }

    zmsg *recv_heartbeat() {
        //  Poll socket for a reply, with timeout
        zmq::pollitem_t items[] = {
            {*m_client_heartbeat, 0, ZMQ_POLLIN, 0}};
        zmq::poll(items, 1, std::chrono::milliseconds(m_timeout));

        //  If we got a reply, process it
        if (items[0].revents & ZMQ_POLLIN) {
            zmsg *msg = new zmsg(*m_client_heartbeat);
            // std::cout << "HEARTBEAT" << std::endl;
            // msg->dump();

            // if (m_verbose) {
            //     s_console("I: received reply:");
            //     msg->dump();
            // }

            if (msg->parts() < 4) {
                std::cout << "Received invalid message from broker" << std::endl;
                msg->dump();
            }
            assert(msg->parts() >= 4);

            assert(msg->pop_front().length() == 0);  // empty message

            ustring header = msg->pop_front();
            // std::cout << "HEADER: " << header.c_str() << std::endl;
            if (header.compare((unsigned char *)k_mdp_client.data()) == 0) {
                ustring service = msg->pop_front();
                assert(service.compare((unsigned char *)service.c_str()) == 0);
                // std::cout << "Service Message: " << std::string(service.begin(), service.end()) << std::endl;
                if (service.compare((unsigned char *)k_mdpc_heartbeat.data()) == 0) {
                    // std::cout << "Received HEARTBEAT from Broker" << std::endl;
                    n_heartbeat_expiry = s_clock() + 2500;
                    cloud_mode = true;
                }
            }

            return msg;  // Success
        }

        if (s_clock() >= m_heartbeat_at) {
            zmsg *message = new zmsg();
            int res = send("HEARTBEAT", "", message);
            // std::cout << "Sending HEARTBEAT to Broker" << std::endl;
            m_heartbeat_at += m_heartbeat;
            // std::cout << "cloud_mode: " << cloud_mode << std::endl;
        }

        if (s_clock() >= n_heartbeat_expiry) {
            // std::cout << "HEARTBEAT EXPIRED" << std::endl;
            cloud_mode = false;
            // std::cout << "cloud_mode: " << cloud_mode << std::endl;
        }

        if (s_interrupted) {
            // std::cout << "W: interrupt received, killing client..." << std::endl;
        } else if (m_verbose) {
            // s_console("W: permanent error, abandoning request");
        }

        return 0;
    }

   private:
    const std::string m_broker;
    const std::string m_broker_heartbeat;
    zmq::context_t *m_context;
    zmq::socket_t *m_client{};  //  Socket to client
    zmq::socket_t *m_client_heartbeat{};  //  Socket to heartbeat
    zmq::socket_t *m_sub_socket{};
    const int m_verbose;     //  Print activity to stdout
    int64_t m_heartbeat_at;  //  When to send HEARTBEAT
    int m_timeout{2500};     //  Request timeout
    int m_heartbeat{2500};   //  Heartbeat delay, msecs
    int64_t n_heartbeat_expiry;
    bool cloud_mode = false;
    std::vector<std::string> client_url_list;
};

#endif