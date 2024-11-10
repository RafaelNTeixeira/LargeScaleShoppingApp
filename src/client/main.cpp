#include <iostream>
#include <zmq.hpp>
#include "ui.cpp"
#include <nlohmann/json.hpp>
#include <uuid/uuid.h>
#include <sqlite3.h>

using json = nlohmann::json;

std::string generateUUID() {
    uuid_t uuid;
    uuid_generate_random(uuid);

    char uuid_str[37];
    uuid_unparse(uuid, uuid_str);

    return std::string(uuid_str);
}

bool isProxyAvailable(zmq::socket_t& socket) {
    try {
        int timeout = 1000; // 1 second
        socket.set(zmq::sockopt::rcvtimeo, timeout);

        // Send a ping message to check if proxy is available
        json request_json;
        request_json["command"] = "PING";

        std::string request_str  = request_json.dump();

        zmq::message_t request(request_str.size());
        memcpy(request.data(), request_str.c_str(), request_str.size());

        socket.send(request, zmq::send_flags::none); // Send message to proxy

        zmq::message_t reply;
        zmq::recv_result_t result = socket.recv(reply, zmq::recv_flags::none); // Wait for reply (or timeout)

        if (result.has_value()) {
            std::cout << "Connected to the proxy, operating in cloud mode..." << std::endl;
            return true; // Proxy is available
        } else {
            std::cout << "Timeout occurred, proxy not available. Operating in local mode..." << std::endl;
            return false; // Proxy is unavailable
        }
    } catch (const zmq::error_t& e) {
        std::cout << "Could not connect to the proxy, operating in local mode..." << std::endl;
        return false; // Proxy is unavailable
    }
}

// Method to print query results
static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
    for (int i = 0; i < argc; i++) {
        std::cout << azColName[i] << ": " << argv[i] << std::endl;
    }
    return 0;
}

int executeSQL(sqlite3* db, const char* sql) {
    char *errMsg = nullptr;
    int rc = sqlite3_exec(db, sql, callback, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
    return rc;
}

void saveListToSQLite(sqlite3* db, const std::string& list_id, const std::string& list_name) {
    std::string sql = "INSERT INTO shopping_lists (list_id, name) VALUES ('" + list_id + "', '" + list_name + "');";
    executeSQL(db, sql.c_str());
    std::cout << "List saved to SQLite database with ID: " << list_id << std::endl;
}

json loadListFromSQLite(sqlite3* db, const std::string& list_id) {
    std::string sql = "SELECT * FROM shopping_lists WHERE list_id = '" + list_id + "';";
    sqlite3_stmt* stmt;
    json list_data;

    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to fetch list: " << sqlite3_errmsg(db) << std::endl;
        return list_data;  // Empty list
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        list_data["list_id"] = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        list_data["name"] = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
    } else {
        std::cerr << "List not found." << std::endl;
    }

    sqlite3_finalize(stmt);
    return list_data;
}

sqlite3* initializeDatabase() {
    sqlite3* db;
    int rc = sqlite3_open("database/local/shopping_lists.db", &db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return nullptr;
    }

    const char* createTableSQL = "CREATE TABLE IF NOT EXISTS shopping_lists ("
                                 "list_id TEXT PRIMARY KEY, "
                                 "name TEXT NOT NULL);";
    executeSQL(db, createTableSQL);
    
    return db;
}

int main() {
    sqlite3* db = initializeDatabase();

    std::cout << "Initialized local db..." << std::endl;
    
    if (!db) {
        return 1;
    }

    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_DEALER);
    socket.connect("tcp://localhost:5556");

    bool connected_to_proxy = isProxyAvailable(socket);

    json request_json;
    int choice;

    while (true) {
        displayMenu();
        std::cin >> choice;

        switch (choice) {
            case 1: {
                std::string base_url = "https://myshoppinglistapp.com/list/";
                // std::string list_id = generateUUID();
                std::string list_id = generateUUID(); // TEMP. FOR TEST PURPOSES
                std::string full_url = base_url + list_id;
                std::string list_name = "My List";
                if (connected_to_proxy) {
                    std::cout << "Create list - CLOUD MODE" << std::endl;
                    request_json["command"] = "CREATE_LIST";
                    request_json["parameters"] = {{"list_id", full_url}, {"name", list_name}};
                }
                else {
                    std::cout << "Create list - LOCAL MODE" << std::endl;
                    saveListToSQLite(db, full_url, list_name);
                }
                break;
            }
            case 2: {
                if (connected_to_proxy) {
                    std::cout << "Get list - CLOUD MODE" << std::endl;
                    request_json["command"] = "GET_LIST";
                } 
                else {
                    std::cout << "Get list - LOCAL MODE" << std::endl;
                    std::cout << "Enter List ID to retrieve: ";
                    std::string list_id;
                    std::cin >> list_id;
                    json list_data = loadListFromSQLite(db, list_id);
                    if (!list_data.empty()) {
                        std::cout << "Retrieved List: " << list_data.dump(4) << std::endl;
                    } else {
                        std::cout << "List not found or unable to load." << std::endl;
                    }
                }
                break;
            }
            case 3: {
                return 0;
            }
            default: {
                std::cout << "Invalid choice. Try again.\n\n";
                continue;
            }
        }

        std::string request = request_json.dump();;
        std::cout << request << std::endl;

        zmq::message_t msg(request.size());
        memcpy(msg.data(), request.c_str(), request.size());
        socket.send(msg, zmq::send_flags::none); // Send message to proxy

        zmq::message_t reply;
        socket.recv(reply, zmq::recv_flags::none); // Receive message from proxy (sent by server)

        std::string response(static_cast<char*>(reply.data()), reply.size());
        std::cout << "Response from server: " << response << std::endl;
    }

    return 0;
}