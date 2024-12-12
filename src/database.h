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
class Database {
   public:
    // Constructor
    Database() {}

    // Load the data
    // In this context, the data are multiple JSON files under file_directory
    // Reads each json file into a key-value pair in the database
    // Where the key is the filename and the value is the content of the file
    // throws an exception if the file cannot be opened printing the whole path
    void load(const std::string& file_directory) {
        std::lock_guard<std::mutex> lock(mutex);
        for (const auto& entry : std::filesystem::directory_iterator(file_directory)) {
            std::ifstream file(entry.path());
            if (file.is_open()) {
                json value;
                file >> value;
                std::string key = entry.path().filename().replace_extension("").string();
                db[key] = value;
                file.close();
            } else {
                throw std::runtime_error("Cannot open file: " + entry.path().string());
            }
        }
    }

    // Save the database to a file
    void save(const std::string& file_directory) {
        std::lock_guard<std::mutex> lock(mutex);
        // remove all files in the directory
        for (const auto& entry : std::filesystem::directory_iterator(file_directory)) {
            std::filesystem::remove(entry.path());
        }
        for (auto& [key, value] : db.items()) {
            std::ofstream file(file_directory + "/" + key + ".json");
            std::cout << "Saving to: " << file_directory + key + ".json" << std::endl;
            if (file.is_open()) {
                file << value.dump(4);
                file.close();
            }
        }
    }

    // Get the value of a key
    // If the key does not exist, return an empty string
    json get(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex);
        if (db.contains(key)) {
            return db[key];
        }
        return "";
    }

    // Set the value of a key
    void set(const std::string& key, const json& value) {
        std::lock_guard<std::mutex> lock(mutex);
        db[key] = value;
    }

    // Delete a key
    void del(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex);
        db.erase(key);
    }

    // Check if a key exists
    bool exists(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex);
        return db.contains(key);
    }

    // Get the number of keys
    int size() {
        std::lock_guard<std::mutex> lock(mutex);
        return db.size();
    }

    // Get all keys
    std::vector<std::string> keys() {
        std::lock_guard<std::mutex> lock(mutex);
        std::vector<std::string> result;
        for (auto& [key, value] : db.items()) {
            result.push_back(key);
        }
        return result;
    }

   private:
    // Mutex for thread safety
    std::mutex mutex;

    // Database object
    json db;
};

std::string getExecutablePath() {
    std::filesystem::path exe_path = std::filesystem::current_path();
    exe_path = std::filesystem::canonical(exe_path);
    return exe_path.string();
}

std::string getDatabasePath(std::string path) {
    return getExecutablePath() + "/" + path;
}

#endif
