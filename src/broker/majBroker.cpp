#include "../zmq/zmsg.hpp"
#include "../zmq/mdp.h"

#include <map>
#include <set>
#include <deque>
#include <list>

static constexpr uint32_t n_heartbeat_liveness = 3;
static constexpr uint32_t n_heartbeat_interval =  2500;    //  msecs
static constexpr uint32_t n_heartbeat_expiry =  n_heartbeat_interval * n_heartbeat_liveness;    //  msecs

struct service;

struct worker {
    std::string m_identity;   //  Address of worker
    service * m_service;      //  Owning service, if known
    int64_t m_expiry;         //  Expires at unless heartbeat

    worker(std::string identity, service * service = nullptr, int64_t expiry = 0): m_identity(identity), m_service(service), m_expiry(expiry) {}
};

// Defines a single service
struct service {
    ~service () {
        for(size_t i = 0; i < m_requests.size(); i++) {
            delete m_requests[i];
        }
    }

    const std::string m_name;       //  Service name
    std::deque<zmsg*> m_requests;   //  List of client requests
    std::list<worker*> m_waiting;   //  List of waiting workers
    size_t m_workers;               //  How many workers we have

    service(std::string name): m_name(name) {}

    void print() const {
        std::cout << "NrRequests: " << m_requests.size() << "; NrWorkers: " << m_waiting.size() << std::endl;
    }
};

class broker {
public:
    //  ---------------------------------------------------------------------
    //  Constructor for broker object
    broker (int verbose): m_verbose(verbose) {
            m_context = new zmq::context_t(1);
            m_client = new zmq::socket_t(*m_context, ZMQ_ROUTER);
            m_worker = new zmq::socket_t(*m_context, ZMQ_ROUTER);
            m_pull_socket = new zmq::socket_t(*m_context, ZMQ_PULL);
            m_xpub_socket = new zmq::socket_t(*m_context, ZMQ_XPUB);
            m_xsub_socket = new zmq::socket_t(*m_context, ZMQ_XSUB);
    }

    //  ---------------------------------------------------------------------
    //  Destructor for broker object
    virtual ~broker () {
        while (! m_services.empty())
        {
            delete m_services.begin()->second;
            m_services.erase(m_services.begin());
        }
        while (! m_workers.empty())
        {
            delete m_workers.begin()->second;
            m_workers.erase(m_workers.begin());
        }
    }

   //  ---------------------------------------------------------------------
   //  Bind broker to endpoints, can call this multiple times
   //  We use a single router socket for both clients and workers.
   void bind (std::string routerWorkerEndpoint, std::string routerClientEndpoint, std::string pullEndpoint, std::string xpubEndpoint) {
        m_routerWorkerEndpoint = routerWorkerEndpoint;
        m_worker->bind(m_routerWorkerEndpoint.c_str());
        s_console ("I: MDP WORKER ROUTER broker/0.1.1 is active at %s", m_routerWorkerEndpoint.c_str());

        m_routerClientEndpoint = routerClientEndpoint;
        m_client->bind(m_routerClientEndpoint.c_str());
        s_console ("I: MDP CLIENT ROUTER broker/0.1.1 is active at %s", m_routerClientEndpoint.c_str());

        m_pullEndpoint = pullEndpoint;
        m_pull_socket->bind(m_pullEndpoint.c_str());
        s_console ("I: MDP PULL broker/0.1.1 is active at %s", m_pullEndpoint.c_str());

        m_xpubEndpoint = xpubEndpoint;
        m_xpub_socket->bind(m_xpubEndpoint.c_str());
        s_console ("I: MDP XPUB broker/0.1.1 is active at %s", m_xpubEndpoint.c_str());

        // m_xsubEndpoint = xsubEndpoint;
        // m_xsub_socket->bind(m_xsubEndpoint.c_str());
        // s_console ("I: MDP XSUB broker/0.1.1 is active at %s", m_xsubEndpoint.c_str());
   }
	
private:
    //  ---------------------------------------------------------------------
    //  Delete any idle workers that haven't pinged us in a while.
    void purge_workers () {
        std::cout << "Entered purge_workers" << std::endl;
        std::deque<worker*> toCull;
        int64_t now = s_clock();
        for (auto wrk = m_waiting.begin(); wrk != m_waiting.end(); ++wrk) {
            std::cout << "Entered first for in purge_workers" << std::endl;
            if ((*wrk)->m_expiry <= now)
                toCull.push_back(*wrk);
        }
        for (auto wrk = toCull.begin(); wrk != toCull.end(); ++wrk) {
            std::cout << "Entered second for in purge_workers" << std::endl;
            if (m_verbose) {
                s_console ("I: deleting expired worker: %s",
                        (*wrk)->m_identity.c_str());
            }
            worker_delete(*wrk, 0);
        }
        std::cout << "Exiting purge_workers" << std::endl;
    }

    //  ---------------------------------------------------------------------
    //  Locate or create new service entry
    service * service_require (std::string name) {
        std::cout << "Entered service_require" << std::endl;
        assert (!name.empty());
        if (m_services.count(name)) {
            std::cout << "Retrieve service" << std::endl;
            return m_services.at(name);
        }
        service * srv = new service(name);
        m_services.insert(std::pair{name, srv});
        std::cout << "Added new service: " << m_services.at(name) << std::endl;
        if (m_verbose) {
            s_console("I: added service: %s", name.c_str());
        }
        return srv;
    }

    //  ---------------------------------------------------------------------
    //  Dispatch requests to waiting workers as possible
    void service_dispatch (service *srv, zmsg *msg) {
        std::cout << "Entered service_dispatch" << std::endl;
        std::cout << "service dispatched: " << srv << std::endl;
        assert (srv);
        if (msg) {                    //  Queue message if any
            std::cout << "Queue message: " << msg << std::endl;
            srv->m_requests.push_back(msg);
            msg->dump();
        }

        purge_workers ();
        std::cout << "service name: " << srv->m_name << std::endl;
        std::cout << "workers waiting count: " << srv->m_waiting.size() << std::endl;
        std::cout << "Requests count: " << srv->m_requests.size() << std::endl;
        while (! srv->m_waiting.empty() && ! srv->m_requests.empty()) {
            // Choose the most recently seen idle worker; others might be about to expire
            auto wrk = srv->m_waiting.begin();
            auto next = wrk;
            for (++next; next != srv->m_waiting.end(); ++next)
            {
                if ((*next)->m_expiry > (*wrk)->m_expiry)
                    wrk = next;
            }
            
            zmsg *msg = srv->m_requests.front();
            std::cout << "Message before worker_send:" << std::endl;
            msg->dump();
            srv->m_requests.pop_front();
            std::cout << "Sent REQUEST to worker:" << std::endl;
            worker_send (*wrk, k_mdpw_request.data(), "", msg);
            m_waiting.erase(*wrk);
            srv->m_waiting.erase(wrk);
            delete msg;
        }
    }

    //  ---------------------------------------------------------------------
    //  Handle internal service according to 8/MMI specification
    void service_internal (std::string service_name, zmsg *msg) {
        std::cout << "Entered service_internal" << std::endl;
        if (service_name.compare("mmi.service") == 0) {
            // service *srv = m_services[msg->body()]; // Dangerous! Silently add key with default value
            service *srv = m_services.count(msg->body()) ? m_services.at(msg->body()) : nullptr;
            if (srv && srv->m_workers) {
                msg->body_set("200");
            } else {
                msg->body_set("404");
            }
        } else {
            msg->body_set("501");
        }

        //  Remove & save client return envelope and insert the
        //  protocol header and service name, then rewrap envelope.
        std::string client = msg->unwrap();
        msg->wrap(k_mdp_client.data(), service_name.c_str());
        msg->wrap(client.c_str(), "");
        msg->send (*m_client);
        delete msg;
    }

    //  ---------------------------------------------------------------------
    //  Creates worker if necessary
    worker * worker_require (std::string identity) {
        assert (!identity.empty());

        //  self->workers is keyed off worker identity
        if (m_workers.count(identity)) {
            return m_workers.at(identity);
        } else {
            worker *wrk = new worker(identity);
            m_workers.insert(std::make_pair(identity, wrk));
            if (m_verbose) {
                s_console ("I: registering new worker: %s", identity.c_str());
            }
            return wrk;
        }
    }

    //  ---------------------------------------------------------------------
    //  Deletes worker from all data structures, and destroys worker
    void worker_delete (worker *&wrk, int disconnect) {
        std::cout << "Entered worker_delete" << std::endl;
        assert (wrk);
        if (disconnect) {
            std::cout << "Sent DISCONNECT to worker" << std::endl;
            worker_send (wrk, k_mdpw_disconnect.data(), "", NULL);
        }

        if (wrk->m_service) {
            for(auto it = wrk->m_service->m_waiting.begin();
                    it != wrk->m_service->m_waiting.end();) {
                if (*it == wrk) {
                    it = wrk->m_service->m_waiting.erase(it);
                }
                else {
                    ++it;
                }
            }
            wrk->m_service->m_workers--;
        }
        m_waiting.erase(wrk);
        // This implicitly calls the worker destructor
        m_workers.erase(wrk->m_identity);
        delete wrk;
    }

    //  ---------------------------------------------------------------------
    //  Process message sent to us by a worker
    void worker_process (std::string sender, zmsg *msg) {
        std::cout << "Entered worker_process" << std::endl;
        assert (msg && msg->parts() >= 1);     //  At least, command

        std::string command = (char *)msg->pop_front().c_str();
        bool worker_ready = m_workers.count(sender)>0;
        worker *wrk = worker_require (sender);

        if (command.compare (k_mdpw_ready.data()) == 0) {
            std::cout << "Received READY from worker" << std::endl;
            if (worker_ready)  { //  Not first command in session
                worker_delete (wrk, 1);
            }
            else {
                if (sender.size() >= 4  //  Reserved service name
                &&  sender.find_first_of("mmi.") == 0) {
                    worker_delete (wrk, 1);
                } else {
                    //  Attach worker to service and mark as idle
                    std::cout << "Attach worker to service and mark as idle" << std::endl;
                    std::string service_name = (char *) msg->pop_front ().c_str();
                    std::cout << "Service name received from worker: " << service_name << std::endl;
                    wrk->m_service = service_require (service_name);
                    wrk->m_service->m_workers++;
                    worker_waiting (wrk);
                }
            }
        } else {
            if (command.compare (k_mdpw_reply.data()) == 0) {
                std::cout << "Received REPLY from worker" << std::endl;
                if (worker_ready) {
                    //  Remove & save client return envelope and insert the
                    //  protocol header and service name, then rewrap envelope.
                    std::string client = msg->unwrap ();
                    msg->wrap (k_mdp_client.data(), wrk->m_service->m_name.c_str());
                    msg->wrap (client.c_str(), "");
                    std::cout << "Sending client:" << std::endl;
                    msg->dump();
                    msg->send (*m_client);
                    worker_waiting (wrk);
                }
                else {
                    worker_delete (wrk, 1);
                }
            } else {
                if (command.compare (k_mdpw_heartbeat.data()) == 0) {
                    std::cout << "Received HEARBEAT from worker" << std::endl;
                    if (worker_ready) {
                        wrk->m_expiry = s_clock () + n_heartbeat_expiry;
                    } else {
                        worker_delete (wrk, 1);
                    }
                } else {
                    if (command.compare (k_mdpw_disconnect.data()) == 0) {
                        std::cout << "Received DISCONNECT from worker" << std::endl;
                        worker_delete (wrk, 0);
                    } else {
                        s_console ("E: invalid input message (%d)", (int) *command.c_str());
                        msg->dump ();
                    }
                }
            }
        }
        delete msg;
    }

    //  ---------------------------------------------------------------------
    //  Send message to worker
    //  If pointer to message is provided, sends that message
    void worker_send (worker *worker, const char *command, std::string option, zmsg *msg) {
        msg = (msg ? new zmsg(*msg) : new zmsg ());

        // Frame 0: Empty frame
        // Frame 1: “MDPW01” (six bytes, representing MDP/Worker v0.1)
        // Frame 2: 0x02 (one byte, representing REQUEST)
        // Frame 3: Client address (envelope stack)
        // Frame 4: Empty (zero bytes, envelope delimiter)
        // Frames 5+: Request body (opaque binary)

        //  Stack protocol envelope to start of message
        if (option.size()>0) {                 //  Optional frame after command
            msg->push_front (option.c_str());
        }

        s_console("I: Pushing command identifier (Frame 2): %s", command);
        msg->push_front(command);

        s_console("I: Pushing protocol identifier (Frame 1): %s", k_mdpw_worker.data());
        msg->push_front(k_mdpw_worker.data());

        //  Stack routing envelope to start of message
        msg->wrap(worker->m_identity.c_str(), "");

        if (m_verbose) {
            s_console ("I: sending %s to worker",
                mdps_commands [(int) *command].data());
            msg->dump ();
        }
        msg->dump ();
        msg->send (*m_worker);
        delete msg;
    }

    //  ---------------------------------------------------------------------
    //  This worker is now waiting for work
    void worker_waiting (worker *worker) {
        std::cout << "Entered worker_waiting" << std::endl;
        assert (worker);
        //  Queue to broker and service waiting lists
        m_waiting.insert(worker);
        worker->m_service->m_waiting.push_back(worker);
        worker->m_expiry = s_clock () + n_heartbeat_expiry;
        // Attempt to process outstanding requests
        service_dispatch (worker->m_service, 0);
    }



    //  ---------------------------------------------------------------------
    //  Process a request coming from a client
    void client_process (std::string sender, zmsg *msg) {
        assert (msg && msg->parts () >= 2);     //  Service name + body

        std::string service_name =(char *) msg->pop_front().c_str();
        std::cout << "Service name received from client: " << service_name << std::endl;
        service *srv = service_require (service_name);
        // Set reply return address to client sender
        msg->wrap (sender.c_str(), "");
        if (service_name.length() >= 4 &&  service_name.find_first_of("mmi.") == 0) {
            service_internal (service_name, msg);
        } else {
            service_dispatch (srv, msg);
        }
    }
        
public:
// Get and process messages forever or until interrupted
void start_brokering() {
    int64_t now = s_clock();
    int64_t heartbeat_at = now + n_heartbeat_interval;
    while (!s_interrupted) {
        zmq::pollitem_t items [] = {
            { *m_pull_socket, 0, ZMQ_POLLIN, 0},
            { *m_client, 0, ZMQ_POLLIN, 0},
            { *m_worker, 0, ZMQ_POLLIN, 0},
            { *m_xpub_socket, 0, ZMQ_POLLIN, 0} };

        int64_t timeout = heartbeat_at - now;
        if (timeout < 0) timeout = 0;
        zmq::poll (items, 4, std::chrono::milliseconds(timeout));

        // Process client list updates
        if (items [0].revents & ZMQ_POLLIN) {
            std::cout << "Entered process client list updates" << std::endl;
            zmsg *msg = new zmsg(*m_pull_socket);

            if (m_verbose) {
                s_console ("I: received message:");
                msg->dump ();
            }

            // IN PROGRESS. NEED A WAY TO GET CLIENT ID THROUGH A PUSH SOCKET SO THAT WE CAN QUEUE A MESSAGE IN THE RIGHT FORMAT
            // DOESN'T RECEIVE SENDER
            std::string sender = (char*)msg->pop_front ().c_str();
            std::cout << "SENDER: " << sender << std::endl;
        }
        // Process client list requests
        if (items [1].revents & ZMQ_POLLIN) {
            std::cout << "Entered process client list requests" << std::endl;
            zmsg *msg = new zmsg(*m_client);
            if (m_verbose) {
                s_console ("I: received message:");
                msg->dump ();
            }
            std::string sender = (char*)msg->pop_front ().c_str();
            std::cout << "SENDER: " << sender << std::endl;
            msg->pop_front (); //empty message
            std::string header = (char*)msg->pop_front ().c_str();

            std::cout << "HEADER: " << header << std::endl;

            if (header.compare(k_mdp_client.data()) == 0) {
                client_process (sender, msg);
            }
            else {
                s_console ("E: invalid message:");
                msg->dump ();
                delete msg;
            }

            std::cout << "Services:" << std::endl;
            for (const auto& pair : m_services) {
                std::cout << "Service Key: " << pair.first << " -> ";
                pair.second->print();
            }
        }
        // Process workers tasks requests
        if (items [2].revents & ZMQ_POLLIN) {
            std::cout << "Entered process workers tasks requests" << std::endl;
            zmsg *msg = new zmsg(*m_worker);
            if (m_verbose) {
                s_console ("I: received message:");
                msg->dump ();
            }
            std::string sender = (char*)msg->pop_front ().c_str();
            std::cout << "SENDER: " << sender << std::endl;
            msg->pop_front (); //empty message
            std::string header = (char*)msg->pop_front ().c_str();

            std::cout << "HEADER: " << header << std::endl;

            if (header.compare(k_mdpw_worker.data()) == 0) {
                worker_process (sender, msg);
            }
            else {
                s_console ("E: invalid message:");
                msg->dump ();
                delete msg;
            }

            std::cout << "Services:" << std::endl;
            for (const auto& pair : m_services) {
                std::cout << "Service Key: " << pair.first << " -> ";
                pair.second->print();
            }
        }
        // Publish to clients list updates
        //   if (items [2].revents & ZMQ_POLLIN) {
        //       std::cout << "Entered publish to clients list updates" << std::endl;
        //       zmsg *msg = new zmsg(*m_xpub_socket);
        //       if (m_verbose) {
        //           s_console ("I: received message:");
        //           msg->dump ();
        //       }
        //       std::string sender = (char*)msg->pop_front ().c_str();
        //       msg->pop_front (); //empty message
        //       std::string header = (char*)msg->pop_front ().c_str();

        //       std::cout << "HEADER: " << header << std::endl;
        //   }
        //  Disconnect and delete any expired workers
        //  Send heartbeats to idle workers if needed
        now = s_clock();
        if (now >= heartbeat_at) {
            purge_workers ();
            for (auto it = m_waiting.begin(); it != m_waiting.end() && (*it)!=0; it++) {
                std::cout << "Sent HEARBEAT to worker" << std::endl;
                worker_send (*it, k_mdpw_heartbeat.data(), "", NULL);
            }
            heartbeat_at += n_heartbeat_interval;
            now = s_clock();
        }
    }
}

private:
    zmq::context_t * m_context;                  //  0MQ context
    zmq::socket_t * m_client;                    //  Socket for clients list requests 
    zmq::socket_t * m_worker;                    //  Socket for workers tasks request
    zmq::socket_t * m_pull_socket;               //  Socket for client list updates
    zmq::socket_t * m_xpub_socket;               //  Socket for broker to publish list updates to clients
    zmq::socket_t * m_xsub_socket;               //  Socket for broker to subscribe to list updates made by workers
    const int m_verbose;                         //  Print activity to stdout
    std::string m_routerClientEndpoint;          //  Broker binds ROUTER socket to this client endpoint
    std::string m_routerWorkerEndpoint;          //  Broker binds ROUTER socket to this worker endpoint
    std::string m_pullEndpoint;                  //  Broker binds PULL socket to this endpoint
    std::string m_xpubEndpoint;                  //  Broker binds XPUB to this endpoint
    std::string m_xsubEndpoint;                  //  Broker binds XSUB to this endpoint
    std::map<std::string, service*> m_services;  //  Hash of known services
    std::map<std::string, worker*> m_workers;    //  Hash of known workers
    std::set<worker*> m_waiting;                 //  List of waiting workers
};