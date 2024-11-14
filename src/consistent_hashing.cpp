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

public:
    ConsistentHashing() = default;

    void addServer(const std::string& server) {
        if (std::find(servers.begin(), servers.end(), server) != servers.end()) {
            std::cout << "Server already added.\n";
            return;
        }

        servers.push_back(server);
        numberOfServers++; 

        size_t hash = generateHash(server);
        ring[hash] = server; 
    }

    void removeServer(const std::string& server) {
        auto it = std::find(servers.begin(), servers.end(), server); 
        if(it == servers.end()) {
            std::cout << "Couldn't find the server.\n"; 
            return;
        }

        servers.erase(it);
        numberOfServers--;

        size_t hash = generateHash(server);
        ring.erase(hash);
    }

    std::string getServer(const std::string& key) {
        if(ring.empty()) {
            return "";
        }

        size_t hash = generateHash(key);
        auto it = ring.lower_bound(hash); 

        if(it == ring.end()) {
            it = ring.begin();
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

}; 
