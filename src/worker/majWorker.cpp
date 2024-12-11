#ifndef __MDWRKAPI_HPP_INCLUDED__
#define __MDWRKAPI_HPP_INCLUDED__

#include "../zmq/zmsg.hpp"
#include "../zmq/mdp.h"
#include "../server/consistent_hashing.cpp"
#include <nlohmann/json.hpp>
#include <uuid/uuid.h>

using json = nlohmann::json;

std::map<size_t, std::string> convert_json_to_ring(std::string json_str) {
    std::map<size_t, std::string> result;
    json j = json::parse(json_str);
    for (auto& el : j.items()) {
        size_t key = std::stoull(el.key());  
        std::string value = el.value();    

        result[key] = value;
    }

    return result;
}

json convert_ring_to_json(std::map<size_t, std::string> ring) {
    json json_ring;
    for (const auto& [key, value] : ring) {
        json_ring[std::to_string(key)] = value;
    }
    return json_ring;
}


//  Structure of our class
//  We access these properties only via class methods
class mdwrk {
public:

   //  ---------------------------------------------------------------------
   //  Constructor
    mdwrk (std::string broker, std::string worker_pub, std::string worker_pull_bind, std::string service, int verbose): m_broker(broker), m_worker_pub_bind(worker_pub), m_worker_pull_bind(worker_pull_bind), m_service(service), m_verbose(verbose) {
        s_version_assert (4, 0);
        m_context = new zmq::context_t (1);
        ch = new ConsistentHashing(1);
        s_catch_signals ();
        connect_to_broker ();
    }

    //  ---------------------------------------------------------------------
    //  Destructor
    virtual ~mdwrk () {
        delete ch;
        delete m_worker;
        delete m_worker_pub;
        delete m_context;
    }

    //  ---------------------------------------------------------------------
    //  Send message to broker
    //  If no _msg is provided, creates one internally
    void send_to_broker(const char *command, std::string option, std::string worker_pull_bind, zmsg *_msg) {
        zmsg *msg = _msg? new zmsg(*_msg): new zmsg ();

        // Frame 0: Empty frame
        // Frame 1: “MDPW01” (six bytes, representing MDP/Worker v0.1)
        // Frame 2: 0x03 (one byte, representing REPLY)
        // Frame 3: Client address (envelope stack)
        // Frame 4: Empty (zero bytes, envelope delimiter)
        // Frames 5+: Reply body (opaque binary)

        //  Stack protocol envelope to start of message
        if (!option.empty()) {
            msg->push_front (option.c_str());
        }
        if (!worker_pull_bind.empty()) {
            msg->push_front (worker_pull_bind.c_str());
        }
        msg->push_front (command);
        msg->push_front (k_mdpw_worker.data());
        msg->push_front ("");

        if (m_verbose) {
            s_console ("I: sending %s to broker",
                mdps_commands [(int) *command].data());
            msg->dump ();
        }
        std::cout << "Sent to broker: " << std::endl;
        msg->dump();
        msg->send (*m_worker);
        delete msg;
    }

    //  ---------------------------------------------------------------------
    //  Send message to worker
    //  If no _msg is provided, creates one internally
    void send_to_worker(const char *command, json ring, zmsg *_msg) {
        zmsg *msg = _msg? new zmsg(*_msg): new zmsg ();

        // Frame 0: Empty frame
        // Frame 1: “MDPW01” (six bytes, representing MDP/Worker v0.1)
        // Frame 2: 0x03 (one byte, representing REPLY)
        // Frame 3: Ring Status (json)

        std::string ring_str = ring.dump();
        msg->push_front (ring_str.c_str());
        msg->push_front (command);
        msg->push_front (k_mdpw_worker.data());
        msg->push_front ("");

        if (m_verbose) {
            s_console ("I: sending %s to worker",
                mdps_commands [(int) *command].data());
            msg->dump ();
        }
        std::cout << "Sent to worker: " << std::endl;
        msg->dump();
        msg->send (*m_worker_push);
        delete msg;
    }

    //  ---------------------------------------------------------------------
    // Publish message to broker XSUB
    //  If no _msg is provided, creates one internally
    void publish_to_broker(std::string url_list, std::string shopping_list, zmsg *_msg) {
        zmsg *msg = _msg? new zmsg(*_msg): new zmsg ();

        // Frame 0: Topic (url of updated list)
        // Frame 1: Updated shopping list

        msg->push_front (shopping_list.c_str());
        msg->push_front (url_list.c_str());

        if (m_verbose) {
            s_console ("I: sending to broker topic", url_list);
            msg->dump ();
        }
        std::cout << "Publish to broker: " << std::endl;
        msg->dump();
        msg->send (*m_worker_pub);
        delete msg;
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
    void connect_to_broker () {
        if (m_worker) {
            delete m_worker;
        }
        m_worker = new zmq::socket_t (*m_context, ZMQ_DEALER);
        int linger = 0;
        m_worker->set(zmq::sockopt::linger, linger);
        std::string worker_id = generateUUID();
        m_worker->set(zmq::sockopt::routing_id, worker_id);
        m_worker->connect (m_broker.c_str());

        m_worker_pub = new zmq::socket_t(*m_context, ZMQ_PUB);
        m_worker_pub->bind(m_worker_pub_bind.c_str());

        m_worker_pull = new zmq::socket_t(*m_context, ZMQ_PULL);
        m_worker_pull->bind(m_worker_pull_bind.c_str());
        std::cout << "PULL socket bound to " << m_worker_pull_bind << std::endl;

        m_worker_push = new zmq::socket_t(*m_context, ZMQ_PUSH);
        std::cout << "PUSH socket created" << std::endl;

        s_console("I: connecting DEALER to broker at %s...", m_broker.c_str());
        s_console("I: binding PUB at %s...", m_worker_pub_bind.c_str());

        // Register service with broker
        std::cout << "Sent READY to broker" << std::endl;
        send_to_broker (k_mdpw_ready.data(), m_service, m_worker_pull_bind, NULL);

        // If liveness hits zero, queue is considered disconnected
        m_liveness = n_heartbeat_liveness;
        m_heartbeat_at = s_clock () + m_heartbeat;
    }

    //  ---------------------------------------------------------------------
    //  Set heartbeat delay
    void set_heartbeat (int heartbeat) {
        m_heartbeat = heartbeat;
    }

    //  ---------------------------------------------------------------------
    //  Set reconnect delay
    void set_reconnect (int reconnect) {
        m_reconnect = reconnect;
    }

    //  ---------------------------------------------------------------------
    //  Send reply, if any, to broker and wait for next request.
    zmsg * recv (zmsg *&reply_p) {
        std::cout << "Entered recv" << std::endl;
        // Format and send the reply if we were provided one
        zmsg *reply = reply_p;
        assert (reply || !m_expect_reply);
        if (reply) {
            std::cout << "Worker has reply:" << std::endl;
            reply->dump();
            assert (m_reply_to.size()!=0);
            reply->wrap (m_reply_to.c_str(), "");
            m_reply_to = "";
            send_to_broker (k_mdpw_reply.data(), "", "", reply);
            delete reply_p;
            reply_p = 0;
        }
        m_expect_reply = true;

        while (!s_interrupted) {
            zmq::pollitem_t items[] = {
                { *m_worker, 0, ZMQ_POLLIN, 0 } , 
                { *m_worker_pull, 0, ZMQ_POLLIN, 0 }
            };
            zmq::poll (items, 2, std::chrono::milliseconds(m_heartbeat));

            if (items[0].revents & ZMQ_POLLIN) {
                zmsg *msg = new zmsg(*m_worker);
                std::cout << "Received message from broker:" << std::endl;
                msg->dump();
                if (m_verbose) {
                    s_console ("I: received message from broker:");
                    msg->dump ();
                }
                m_liveness = n_heartbeat_liveness;

                assert (msg->parts () >= 3);

                ustring empty = msg->pop_front ();
                std::string empty_str = (char*) empty.c_str();
                std::cout << "empty:" << empty_str << std::endl;

                assert (empty.compare((unsigned char *)"") == 0);
                //assert (strcmp (empty, "") == 0);
                //free (empty);

                ustring header = msg->pop_front ();
                std::string header_str = (char*) header.c_str();
                std::cout << "header:" << header_str << std::endl;
                assert (header.compare((unsigned char *)k_mdpw_worker.data()) == 0);
                //free (header);

                std::string command =(char*) msg->pop_front ().c_str();
                std::cout << "command:" << command << std::endl;
                if (command.compare (k_mdpw_request.data()) == 0) {
                    std::cout << "Reply to:" << std::endl;
                    msg->dump();
                    m_reply_to = msg->unwrap ();

                    ustring request_type = msg->pop_front();
                    std::string request_type_str = (char*) request_type.c_str();
                    std::cout << "request_type received: " << request_type_str << std::endl;

                    ustring url_list = msg->pop_front();
                    std::string url_list_str = (char*) url_list.c_str();
                    std::cout << "url_list received: " << url_list_str << std::endl;

                    Response res = handleRequest(url_list_str, request_type_str, msg);
                    
                    publish_to_broker(url_list_str, res.shopping_list, NULL);

                    return new zmsg(res.reply.c_str());     //  We have a request to process
                }
                else if (command.compare (k_mdpw_broadcast_ring.data()) == 0) {
                    std::cout << "Entered k_mdpw_broadcast_ring" << std::endl;

                    std::string ring_str = (char*) msg->pop_front().c_str();
                    std::cout << "Ring received: " << ring_str << std::endl;

                    std::map<size_t, std::string> ring = convert_json_to_ring(ring_str);
                }
                else if (command.compare (k_mdpw_join_ring.data()) == 0) {
                    std::cout << "Entered k_mdpw_join_ring" << std::endl;

                    std::string ring_str = (char*) msg->pop_front().c_str();
                    std::cout << "Ring received: " << ring_str << std::endl;

                    std::map<size_t, std::string> ring = convert_json_to_ring(ring_str);
                }
                else if (command.compare (k_mdpw_heartbeat.data()) == 0) {
                    // Do nothing for heartbeats
                }
                else if (command.compare (k_mdpw_disconnect.data()) == 0) {
                    connect_to_broker ();
                }
                else {
                    s_console ("E: invalid input message (%d)",
                          (int) *(command.c_str()));
                    msg->dump ();
                }
                delete msg;
            } 
            if (items[1].revents & ZMQ_POLLIN) {
                zmsg *msg = new zmsg(*m_worker_pull);
                std::cout << "Received message from worker:" << std::endl;
                msg->dump();
                if (m_verbose) {
                    s_console ("I: received message from worker:");
                    msg->dump ();
                }
                assert (msg->parts () >= 3);

                ustring empty = msg->pop_front ();
                std::string empty_str = (char*) empty.c_str();
                std::cout << "empty:" << empty_str << std::endl;

                assert (empty.compare((unsigned char *)"") == 0);
                //assert (strcmp (empty, "") == 0);
                //free (empty);

                ustring header = msg->pop_front ();
                std::string header_str = (char*) header.c_str();
                std::cout << "header:" << header_str << std::endl;
                assert (header.compare((unsigned char *)k_mdpw_worker.data()) == 0);
                //free (header);

                std::string command =(char*) msg->pop_front ().c_str();
                std::cout << "command:" << command << std::endl;
                
                // if (command.compare (k_mdpw_join_ring.data()) == 0) {
                //     std::cout << "In rec join ring cmd" << std::endl;
                //     ch->addServer(m_worker_pull_bind);
                //     std::cout << "Number of workers in the ring: " << ch->getNumberOfServers() << std::endl;
                // } else if (command.compare (k_mdpw_broadcast_ring.data()) == 0) {
                //     std::cout << "Received broacast ring" << std::endl;
                //     msg->dump();
                //     ustring ring_ = msg->pop_front(); 
                //     std::cout << "RING RECV: " << ring_.c_str() << std::endl;
                //     std::map<size_t, std::string> ring = convert_json_to_ring(ring_); 
                //     ch->setRing(ring);
                //     std::cout << "New number of workers in the ring: " << ch->getNumberOfServers() << std::endl;
                // }     
            }
            else
            if (--m_liveness == 0) {
                if (m_verbose) {
                    s_console ("W: disconnected from broker - retrying...");
                }
                s_sleep (m_reconnect);
                connect_to_broker ();
            }
            //  Send HEARTBEAT if it's time
            if (s_clock () >= m_heartbeat_at) {
                send_to_broker (k_mdpw_heartbeat.data(), "", "", NULL);
                m_heartbeat_at += m_heartbeat;
            }
        }
        if (s_interrupted)
            printf ("W: interrupt received, killing worker...\n");
        return NULL;
    }

    std::string get_worker_pull_bind() {
        return m_worker_pull_bind;
    }

    zmq::socket_t* get_worker_push() {
        return m_worker_push;
    }
    zmq::socket_t* get_worker_pull() {
        return m_worker_pull;
    }

    ConsistentHashing* getConsistentHashing() {
        return ch;
    }

private:
    static constexpr uint32_t n_heartbeat_liveness = 3; // 3-5 is reasonable
    const std::string m_broker;
    const std::string m_worker_pub_bind;
    const std::string m_worker_pull_bind;
    const std::string m_service;
    zmq::context_t *m_context;
    zmq::socket_t *m_worker{};  //  Socket to broker
    zmq::socket_t *m_worker_pub{};  //  Bind for PUB socket
    zmq::socket_t *m_worker_push{};  //  Bind for PUSH socket
    zmq::socket_t *m_worker_pull{};  //  Bind for PULL socket
    const int m_verbose;         //  Print activity to stdout

    // Heartbeat management
    int64_t m_heartbeat_at;      //  When to send HEARTBEAT
    size_t m_liveness;           //  How many attempts left
    int m_heartbeat{2500};       //  Heartbeat delay, msecs
    int m_reconnect{2500};       //  Reconnect delay, msecs

    // Internal state
    bool m_expect_reply{false};  //  Zero only at start

    // Return address, if any
    std::string m_reply_to;

    ConsistentHashing* ch;
};

#endif