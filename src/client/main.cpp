#include <sqlite3.h>
#include <uuid/uuid.h>
#include <atomic>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <zmq.hpp>
#include "../crdt/shopping_list.h"
#include "majClient.cpp"
#include "ui.cpp"

using json = nlohmann::json;

// std::atomic<bool> connected_to_proxy(false);

using json = nlohmann::json;

template <typename K>
json serializeGSet(const GSet<K>& gset) {
    json j;
    for (const auto& entry : gset.elements()) {
        const K& element = entry;

        int count = gset.read(element);
        j[element] = count;
    }
    return j;
}

bool isProxyAvailable(zmq::socket_t& socket) {
    try {
        int timeout = 1000;  // 1 second
        socket.set(zmq::sockopt::rcvtimeo, timeout);

        // Send a ping message to check if proxy is available
        json request_json;
        request_json["command"] = "PING";

        std::string request_str = request_json.dump();

        zmq::message_t request(request_str.size());
        memcpy(request.data(), request_str.c_str(), request_str.size());

        socket.send(request, zmq::send_flags::none);  // Send message to proxy

        zmq::message_t reply;
        zmq::recv_result_t result = socket.recv(reply, zmq::recv_flags::none);  // Wait for reply (or timeout)

        if (result.has_value()) {
            std::cout << "Connected to the proxy, operating in cloud mode..." << std::endl;
            return true;  // Proxy is available
        } else {
            std::cout << "Timeout occurred, proxy not available. Operating in local mode..." << std::endl;
            return false;  // Proxy is unavailable
        }
    } catch (const zmq::error_t& e) {
        std::cout << "Could not connect to the proxy, operating in local mode..." << std::endl;
        return false;  // Proxy is unavailable
    }
}

// Method to print query results
static int callback(void* NotUsed, int argc, char** argv, char** azColName) {
    for (int i = 0; i < argc; i++) {
        std::cout << azColName[i] << ": " << argv[i] << std::endl;
    }
    return 0;
}

int executeSQL(sqlite3* db, const char* sql) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql, callback, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
    return rc;
}

void saveListToLocal(sqlite3* db, ShoppingList& shoppingList) {
    std::string sql = "INSERT INTO shopping_lists (url, name) VALUES ('" + shoppingList.getURL() + "', '" + shoppingList.getTitle() + "');";
    executeSQL(db, sql.c_str());
    std::cout << "List saved to SQLite database with ID: " << shoppingList.getURL() << std::endl;

    for (const auto& item : shoppingList.getHistory()) {
        std::string item_sql = "INSERT INTO shopping_lists_items (list_url, item_name, acquired_quantity) VALUES ('" + shoppingList.getURL() + "', '" + item.target + "', " + std::to_string(item.quantity) + ");";
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
ShoppingListResponse addProductsToList(ShoppingList& shoppingList) {
    ShoppingListResponse shoppingListResponse;
    std::string product_name;
    int quantity;

    while (true) {
        std::cout << "Enter product name (or type 'q' to finish): ";
        std::cin >> product_name;

        if (product_name == "q") {
            break;
        }

        std::cout << "Enter quantity for " << product_name << ": ";
        std::cin >> quantity;

        shoppingListResponse = shoppingList.createItem(product_name, quantity);
    }

    return shoppingListResponse;
}

// void checkProxyConnection(zmq::socket_t& socket) {
//     while (true) {
//         connected_to_proxy = isProxyAvailable(socket);
//         std::this_thread::sleep_for(std::chrono::seconds(5)); // Check every 5 seconds
//     }
// }

// Function receive shopping list updates from the SUB socket
void listenForUpdates(mdcli& client) {
    while (true) {
        std::vector<std::string> sub_update = client.receive_updates();
        if (!sub_update.empty()) {
            std::cout << "List update notification received:" << std::endl;
            for (const auto& update : sub_update) {
                std::cout << " - " << update << std::endl;
            }
        }
    }
}

// Function receive heartbeats from the broker
void listenForHeartBeats(mdcli& client) {
    while (true) {
        zmsg* heartbeat = client.recv();
    }
}

int main() {
    sqlite3* db = initializeDatabase();

    std::cout << "Initialized local db..." << std::endl;

    if (!db) {
        return 1;
    }

    s_catch_signals();

    mdcli client("tcp://localhost:5556", 1);

    std::thread update_listener(listenForUpdates, std::ref(client));
    update_listener.detach(); // Ensures the thread runs independently

    std::thread update_heartbeat(listenForHeartBeats, std::ref(client));
    update_heartbeat.detach(); // Ensures the thread runs independently

    json request_json;
    int choice;

    while (s_interrupted == 0) {
        displayMenu();
        std::cout << "Enter choice: ";
        std::string list_url_client_input = "";
        
        while (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid input. Please enter a valid choice: ";
        }

        switch (choice) {
            case 1: {
                std::string base_url = "https://myshoppinglistapp.com/list/";
                std::string list_id = mdcli::generateUUID();
                std::string full_url = base_url + list_id;

                std::cout << "Give a name to your list: ";
                std::string list_name;
                std::cin >> list_name;

                // ShoppingList shoppingList(list_id, list_name, full_url);

                // ShoppingListResponse shoppingListResponse = addProductsToList(shoppingList);
                // std::cout << shoppingList.getItems() << std::endl;
                // json shoppingListJson = serializeGSet(shoppingList.getItems());

                std::cout << "Create list - CLOUD MODE" << std::endl;
                break;
            }
            case 2: {
                std::cout << "Enter List URL to retrieve: ";
                std::cin >> list_url_client_input;

                std::cout << "Get list - CLOUD MODE" << std::endl;
                break;
            }
            case 3: {
                std::cout << "Thank You for using our platform!" << std::endl;
                s_interrupted = 1;
                break;
            }
            default: {
                std::cout << "Invalid choice. Try again.\n\n";
                continue;
            }
        }

        bool cloud_mode = client.get_cloud_mode();
        if (choice == 1 || choice == 2) {
            if (choice == 1) {
                std::cout << "cloud_mode: " << cloud_mode << std::endl;
                std::string generated_url = "zxcv";
                zmsg* msg = new zmsg(generated_url.c_str());
                client.send("LIST_MANAGEMENT", "CREATE_LIST", msg);
                delete msg;
            } else if (choice == 2) {
                // Ask for a list (DEALER)
                std::cout << "cloud_mode: " << cloud_mode << std::endl;
                const char* url_list_msg_parameter = list_url_client_input.c_str();
                zmsg* msg = new zmsg(url_list_msg_parameter);
                client.send("LIST_MANAGEMENT", "GET_LIST", msg);
            }

            if (cloud_mode == 1){
                zmsg* reply = client.recv();
                if (reply) {
                    std::cout << "Reply received: " <<  std::endl;
                    reply->dump();

                    ustring temp = reply->pop_front();
                    if (temp.empty()) {
                        std::cerr << "Error: Received empty reply!" << std::endl;
                    } else {
                        std::string response(reinterpret_cast<const char*>(temp.c_str()), temp.size());
                        std::cout << "Response from server: " << response << std::endl;
                    }
                    
                    delete reply;
                } else {
                    std::cout << "No response received from the server." << std::endl;
                }
            }
        }
    }

    return 0;
}
