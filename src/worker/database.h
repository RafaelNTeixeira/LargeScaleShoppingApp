#ifndef DATABASE_H
#define DATABASE_H

#include <fstream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

using json = nlohmann::json;

// Database API for JSON-based database files
// The database object will be shared among all worker threads
// So we need to make sure that the database object is thread-safe
class Database {
   public:
    // Constructor
    Database();

    // Destructor
    ~Database();

    // Load the database from a file
    void load(const std::string& filename);

    // Save the database to a file
    void save(const std::string& filename);

    // Get the value of a key
    // If the key does not exist, return an empty string
    std::string get(const std::string& key);

    // Set the value of a key
    void set(const std::string& key, const std::string& value);

    // Delete a key
    void del(const std::string& key);

    // Check if a key exists
    bool exists(const std::string& key);

    // Get the number of keys
    int size();

    // Get all keys
    std::vector<std::string> keys();

   private:
    // Mutex for thread safety
    std::mutex mutex;

    // Database object
    json db;
};

// Implement
Database::Database() {
    db = json::object();
}

Database::~Database() {
    // Do nothing
}

void Database::load(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex);
    std::ifstream file(filename);
    if (file.is_open()) {
        file >> db;
        file.close();
    }
}

void Database::save(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex);
    std::ofstream file(filename);
    if (file.is_open()) {
        file << db.dump(4);
        file.close();
    }
}

std::string Database::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex);
    if (db.contains(key)) {
        return db[key];
    }
    return "";
}

void Database::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex);
    db[key] = value;
}

void Database::del(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex);
    db.erase(key);
}

bool Database::exists(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex);
    return db.contains(key);
}

int Database::size() {
    std::lock_guard<std::mutex> lock(mutex);
    return db.size();
}

std::vector<std::string> Database::keys() {
    std::lock_guard<std::mutex> lock(mutex);
    std::vector<std::string> result;
    for (auto& [key, value] : db.items()) {
        result.push_back(key);
    }
    return result;
}

#endif
