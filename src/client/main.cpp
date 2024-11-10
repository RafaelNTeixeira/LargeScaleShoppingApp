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

void saveListToLocal(sqlite3* db, const std::string& list_url, const std::string& list_name, const std::unordered_map<std::string, int>& items) {
    std::string sql = "INSERT INTO shopping_lists (url, name) VALUES ('" + list_url + "', '" + list_name + "');";
    executeSQL(db, sql.c_str());
    std::cout << "List saved to SQLite database with ID: " << list_url << std::endl;

    for (const auto& item : items) {
        std::string item_sql = "INSERT INTO shopping_lists_items (list_url, item_name, acquired_quantity) VALUES ('" + list_url + "', '" + item.first + "', " + std::to_string(item.second) + ");";
        executeSQL(db, item_sql.c_str());
    }
}

json loadListFromLocal(sqlite3* db, const std::string& list_url) {
    std::string sql = "SELECT * FROM shopping_lists WHERE url = '" + list_url + "';";
    sqlite3_stmt* stmt;
    json list_data;

    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to fetch list: " << sqlite3_errmsg(db) << std::endl;
        return list_data;
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        list_data["url"] = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        list_data["name"] = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
    } else {
        std::cerr << "List not found." << std::endl;
    }

    sqlite3_finalize(stmt);


    std::string sql_items = "SELECT item_name, acquired_quantity FROM shopping_lists_items WHERE list_url = '" + list_url + "';";
    rc = sqlite3_prepare_v2(db, sql_items.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to fetch items: " << sqlite3_errmsg(db) << std::endl;
        return list_data;
    }

    json items = json::array();
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        json item;
        item["item_name"] = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        item["acquired_quantity"] = sqlite3_column_int(stmt, 1);
        items.push_back(item);
    }

    sqlite3_finalize(stmt);

    list_data["products"] = items;

    return list_data;
}

sqlite3* initializeDatabase() {
    sqlite3* db;
    int rc = sqlite3_open("database/local/shopping_lists.db", &db);
    
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return nullptr;
    }

    const char* createTableSQL1 = 
        "CREATE TABLE IF NOT EXISTS shopping_lists ("
        "url TEXT PRIMARY KEY, "
        "name TEXT NOT NULL);";
    executeSQL(db, createTableSQL1);

    const char* createTableSQL2 = 
        "CREATE TABLE IF NOT EXISTS shopping_lists_items ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "list_url TEXT NOT NULL, "
        "item_name TEXT NOT NULL, "
        "acquired_quantity INTEGER DEFAULT 0, "
        "FOREIGN KEY(list_url) REFERENCES shopping_lists(list_url));"; 
    executeSQL(db, createTableSQL2);

    return db;
}

// Function to collect products from the user
std::unordered_map<std::string, int> addProductsToList() {
    std::unordered_map<std::string, int> items;
    std::string product_name;
    int quantity;

    while (true) {
        std::cout << "Enter product name (or type 'quit' to finish): ";
        std::cin >> product_name;

        if (product_name == "quit") {
            break;
        }

        std::cout << "Enter quantity for " << product_name << ": ";
        std::cin >> quantity;

        items[product_name] = quantity;
    }

    return items;
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
                std::string list_id = generateUUID();
                std::string full_url = base_url + list_id;

                std::cout << "Give a name to your list: ";
                std::string list_name;
                std::cin >> list_name;

                std::unordered_map<std::string, int> items = addProductsToList();

                if (connected_to_proxy) {
                    std::cout << "Create list - CLOUD MODE" << std::endl;
                    request_json["command"] = "CREATE_LIST";
                    request_json["parameters"] = {{"list_url", full_url}, {"list_name", list_name}, {"list_items", items}};
                }
                else {
                    std::cout << "Create list - LOCAL MODE" << std::endl;
                    saveListToLocal(db, full_url, list_name, items);
                }
                break;
            }
            case 2: {
                std::cout << "Enter List URL to retrieve: ";
                std::string list_url;
                std::cin >> list_url;
                if (connected_to_proxy) {
                    std::cout << "Get list - CLOUD MODE" << std::endl;
                    request_json["command"] = "GET_LIST";
                    request_json["parameters"] = {{"list_url", list_url}};;
                } 
                else {
                    std::cout << "Get list - LOCAL MODE" << std::endl;
                    json list_data = loadListFromLocal(db, list_url);
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
        socket.send(msg, zmq::send_flags::none);

        zmq::message_t reply;
        socket.recv(reply, zmq::recv_flags::none);

        std::string response(static_cast<char*>(reply.data()), reply.size());
        std::cout << "Response from server: " << response << std::endl;
    }

    return 0;
}
