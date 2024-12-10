#include <iostream>
#include <vector>
#include <map>
#include <cstdint>
#include <functional>

size_t generateHash(const std::string &key) {
    std::hash<std::string> hasher;
    return hasher(key); 
}

class ConsistentHashing {
private: 
    std::map<size_t, std::string> ring; // <hash, server_name>
    std::vector<std::string> servers;
    int numberOfServers = 0;
    int numberOfVirtualNodes;

public:
    ConsistentHashing(int virtualNodes=3) : numberOfVirtualNodes(virtualNodes) {}
     
    int getNumberOfVirtualNodes() const {
        return numberOfVirtualNodes;
    }

    void addServer(const std::string& server) {
        if (std::find(servers.begin(), servers.end(), server) != servers.end()) {
            std::cout << "Server already added.\n";
            return;
        }

        servers.push_back(server);
        numberOfServers++; 

        for(int i = 0; i < numberOfVirtualNodes; i++) {
           size_t hash = generateHash(server + std::to_string(i));
            ring[hash] = server + ":" + std::to_string(i); 
        }
    }

    void removeServer(const std::string& server) {
        auto it = std::find(servers.begin(), servers.end(), server); 
        if(it == servers.end()) {
            std::cout << "Couldn't find the server.\n"; 
            return;
        }

        servers.erase(it);
        numberOfServers--;

        for(int i = 0; i < numberOfVirtualNodes; i++) {
            size_t hash = generateHash(server + std::to_string(i));
            ring.erase(hash);
        }
    }

    std::string getServer(const std::string &key) {
        if (ring.empty()) {
            return "";
        }

        size_t hash = generateHash(key);
        auto it = ring.lower_bound(hash);
        if (it == ring.end()) {
            it = ring.begin(); 
        }
        return it->second.substr(0, it->second.find(":")); 
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

    int getNumberOfServers() {
        return numberOfServers; 
    }

    void printRing() {
        for(const auto& server : ring) {
            std::cout  << "Hash: " << server.first << " / Server's name: " << server.second << std::endl;
        }
    }

    std::map<size_t, std::string> getRing() {
        return ring;
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