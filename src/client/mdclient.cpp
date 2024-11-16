#ifndef __MDCLIAPI_HPP_INCLUDED__
#define __MDCLIAPI_HPP_INCLUDED__

#include "../zmq/zmsg.hpp"
#include "../zmq/mdp.h"

//  Structure of our class
//  We access these properties only via class methods
class mdcli {
public:
   //  ---------------------------------------------------------------------
   //  Constructor
   mdcli (std::string broker, int verbose): m_broker(broker), m_verbose(verbose) {
       s_version_assert (4, 0);
       m_context = new zmq::context_t (1);
       s_catch_signals ();
       connect_to_broker ();
       connect_push_socket();
       connect_sub_socket();
   }

   //  ---------------------------------------------------------------------
   //  Destructor
   virtual ~mdcli () {
       delete m_client;
       delete m_context;
       delete m_push_socket;
       delete m_sub_socket;
   }

   //  ---------------------------------------------------------------------
   //  Connect or reconnect to broker
   void connect_to_broker () {
       if (m_client) {
           delete m_client;
       }
       m_client = new zmq::socket_t (*m_context, ZMQ_DEALER);
       int linger = 0;
       m_client->set(zmq::sockopt::linger, linger);
       s_set_id(*m_client);
       m_client->connect (m_broker.c_str());
       if (m_verbose)
           s_console ("I: connecting to broker at %s...", m_broker.c_str());
   }

   // Initialize and connect PUSH socket
   void connect_push_socket() {
       m_push_socket = new zmq::socket_t(*m_context, ZMQ_PUSH);
       m_push_socket->connect("tcp://localhost:5557");
       std::cout << "Connected to broker pull socket" <<std::endl;
   }

   // Initialize and connect SUB socket
   void connect_sub_socket() {
       m_sub_socket = new zmq::socket_t(*m_context, ZMQ_SUB);
       m_sub_socket->connect("tcp://localhost:5558");
       m_sub_socket->set(zmq::sockopt::subscribe, ""); // Subscribe to all updates with an empty filter
       std::cout << "Connected to broker pub socket" <<std::endl;
   }

   // Send list update via PUSH socket
   void send_update(const std::string& update) {
       zmq::message_t msg(update.size());
       memcpy(msg.data(), update.c_str(), update.size());
       m_push_socket->send(msg, zmq::send_flags::none);
       std::cout << "Sent request through push socket" << std::endl;
   }

   // Receive update notification via SUB socket
   std::string receive_update() {
       zmq::message_t sub_msg;
       if (m_sub_socket->recv(sub_msg, zmq::recv_flags::none)) {
            std::cout << "Received active update through push socket" << sub_msg << std::endl;
           return std::string(static_cast<char*>(sub_msg.data()), sub_msg.size());
       }
       return "";
   }


   //  ---------------------------------------------------------------------
   //  Set request timeout
   void set_timeout (int timeout) {
       m_timeout = timeout;
   }


   //  ---------------------------------------------------------------------
   //  Send request to broker
   //  Takes ownership of request message and destroys it when sent.
   int send (std::string service, zmsg *&request_p) {
        assert (request_p);
        zmsg *request = request_p;

        //  Prefix request with protocol frames
        //  Frame 0: empty (REQ emulation)
        //  Frame 1: "MDPCxy" (six bytes, MDP/Client x.y)
        //  Frame 2: Service name (printable string)
        request->push_front (service.c_str());
        request->push_front (k_mdp_client.data());
        request->push_front ("");
        if (m_verbose) {
            s_console ("I: send request to '%s' service:", service.c_str());
            request->dump ();
        }
        if (service == "CREATE_LIST") {
            request->send (*m_push_socket);
            std::string endpoint = m_push_socket->get(zmq::sockopt::last_endpoint);
            std::cout << "Sent list update through PUSH socket: " << endpoint << std::endl;
        }
        if (service == "GET_LIST") {
            request->send (*m_client);
            std::string endpoint = m_client->get(zmq::sockopt::last_endpoint);
            std::cout << "Sent load list request through DEALER socket: " << endpoint << std::endl;
        }
        return 0;
   }

   //  ---------------------------------------------------------------------
   //  Returns the reply message or NULL if there was no reply. Does not
   //  attempt to recover from a broker failure, this is not possible
   //  without storing all unanswered requests and resending them all...
   zmsg* recv () {
       //  Poll socket for a reply, with timeout
       zmq::pollitem_t items[] = {
           { *m_client, 0, ZMQ_POLLIN, 0 } };
       zmq::poll (items, 1, std::chrono::milliseconds(m_timeout));

       //  If we got a reply, process it
       if (items[0].revents & ZMQ_POLLIN) {
           zmsg *msg = new zmsg (*m_client);
           if (m_verbose) {
               s_console ("I: received reply:");
               msg->dump ();
           }
           //  Don't try to handle errors, just assert noisily
           assert (msg->parts () >= 4);

           assert (msg->pop_front ().length() == 0);  // empty message

           ustring header = msg->pop_front();
           assert (header.compare((unsigned char *)k_mdp_client.data()) == 0);

           ustring service = msg->pop_front();
           assert (service.compare((unsigned char *)service.c_str()) == 0);

           return msg;     //  Success
       }
       
       if (s_interrupted)
           std::cout << "W: interrupt received, killing client..." << std::endl;
       else if (m_verbose)
           s_console ("W: permanent error, abandoning request");

       return 0;
   }

private:
   const std::string m_broker;
   zmq::context_t * m_context;
   zmq::socket_t * m_client{};  //  Socket to broker
   zmq::socket_t  * m_push_socket{};
   zmq::socket_t  * m_sub_socket{};
   const int m_verbose;         //  Print activity to stdout
   int m_timeout{2500};         //  Request timeout
};

#endif