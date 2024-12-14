#include <iostream>
#include <vector>
#include <map>
#include <cstdint>
#include <functional>
#include "../zmq/zmsg.hpp"

size_t generateHash(const std::string &key) {
    std::hash<std::string> hasher;
    return hasher(key); 
}

class ConsistentHashing {
private: 
    std::map<size_t, std::string> ring; // <hash, pull_port>
    std::vector<std::string> servers;
    std::map<size_t, zmq::socket_t *> push_sockets;
    int numberOfVirtualNodes;

public:
    ConsistentHashing(int virtualNodes=1) : numberOfVirtualNodes(virtualNodes) {}
     
    int getNumberOfVirtualNodes() const {
        return numberOfVirtualNodes;
    }

    std::vector<zmq::socket_t *> getPushSockets() {
        std::vector<zmq::socket_t *> ring_push_sockets;

        for (auto node : push_sockets) {
            ring_push_sockets.push_back(node.second);
        }
        
        return ring_push_sockets;
    }

    void addPushSocket(const size_t& hash, zmq::socket_t* socket) {
        if(this->getPushSocket(hash) != nullptr) {
            std::cerr << "Error: Trying to add socket to already known server " << hash << std::endl;
        }
        push_sockets[hash] = socket;
    }

    zmq::socket_t* getPushSocket(const size_t& hash) {
        if(this->push_sockets.find(hash) == this->push_sockets.end()) {
            return nullptr;
        }
        return this->push_sockets[hash];
    }

    void addPushSocketViaServer(const std::string& key, zmq::socket_t* socket) {
        size_t hash = generateHash(key);
        this->addPushSocket(hash, socket);
    }

    zmq::socket_t* getPushSocketViaServer(const std::string& key) {
        size_t hash = generateHash(key);
        return this->getPushSocket(hash);
    }

    void addServer(const std::string& server) {
        if (std::find(servers.begin(), servers.end(), server) != servers.end()) {
            std::cout << "Server already added.\n";
            return;
        }

        servers.push_back(server);
        size_t hash = generateHash(server);
        ring[hash] = server;
        // for(int i = 0; i < numberOfVirtualNodes; i++) {
        //     size_t hash = generateHash(server + std::to_string(i));
        //     ring[hash] = server + "." + std::to_string(i); 
        // }
    }

    void removeServer(const std::string& server) {
        auto it = std::find(servers.begin(), servers.end(), server); 
        if(it == servers.end()) {
            std::cout << "Couldn't find the server.\n"; 
            return;
        }

        servers.erase(it);

        for(int i = 0; i < numberOfVirtualNodes; i++) {
            size_t hash = generateHash(server + std::to_string(i));
            ring.erase(hash);
        }
    }

    std::string getServer(const std::string &key) {
        if (ring.empty()) {
            std::cout << "EMPTY RING" << std::endl;
            return "";
        }

        size_t hash = generateHash(key);
        auto it = ring.lower_bound(hash);
        if (it == ring.end()) {
            it = ring.begin(); 
        }
        return it->second; 
    }

    std::string getServerVirtual(const std::string &key) {
        if (ring.empty()) {
            return ""; 
        }

        size_t hash = generateHash(key);
        auto it = ring.lower_bound(hash);
        if (it == ring.end()) {
            it = ring.begin(); 
        }
        return it->second;
    }

     std::string getNextServer(const std::string &key) {
        if (ring.empty()) {
            return "";
        }

        size_t hash = generateHash(key);
        auto it = ring.lower_bound(hash);
        if (it == ring.end()) {
            it = ring.begin(); 
        }

        auto nextIt = std::next(it);
        if (nextIt == ring.end()) {
            nextIt = ring.begin(); 
        }
        return nextIt->second;
    }

    std::string getPreviousServer(const std::string &key) {
        if (ring.empty()) {
            return "";
        }

        size_t hash = generateHash(key);
        auto it = ring.lower_bound(hash);
        if (it == ring.end()) {
            it = ring.begin(); 
        }

        if (it == ring.begin()) {
            it = std::prev(ring.end()); 
        } else {
            it = std::prev(it);
        }
        return it->second;
    }

    void printRing() {
        for(const auto& server : ring) {
            std::cout  << "Hash: " << server.first << " / Server's name: " << server.second << std::endl;
        }
    }

    std::map<size_t, std::string> getRing() {
        return ring;
    }

    void setRing(std::map<size_t, std::string> ring_) {
        this->ring = ring_;
    }

    void setServers(std::vector<std::string> servers_) {
        this->servers = servers_;
    }

    void updateCH(std::map<size_t, std::string> ring_, std::vector<std::string> servers_) {
        this->ring = ring_; 
        this->servers = servers_; 
    }

    std::vector<std::string> getAllServers() {
        return this->servers;
    }

}; 

/*
int main() {
    ConsistentHashing ch(3);
    ch.addServer("192.168.1.1:8080");
    ch.addServer("192.168.1.2:8080");
    ch.addServer("192.168.1.3:8080");

    std::string key = "my_key";
    std::cout << "Server for key: " << key << " -> " << ch.getServer(key) << std::endl;

    return 0;
}
*/