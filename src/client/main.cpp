#include <iostream>
#include <string>
#include <zmq.hpp>
#include "ui.cpp"
#include <nlohmann/json.hpp>
#include <uuid/uuid.h>
#include <sqlite3.h>
#include <thread>
#include <atomic>
#include "../crdt/shopping_list.h"
#include "mdclient.cpp"

using json = nlohmann::json;

//std::atomic<bool> connected_to_proxy(false);

using json = nlohmann::json;

template <typename K>
json serializeGSet(const GSet<K>& gset) {
    json j;
    for (const auto& entry : gset.elements()) {
        const K& element = entry;
        // Assuming Counter has a method to retrieve its value
        int count = gset.read(element);  
        j[element] = count;
    }
    return j;
}

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

int main() {
    sqlite3* db = initializeDatabase();

    std::cout << "Initialized local db..." << std::endl;
    
    if (!db) {
        return 1;
    }

    mdcli client("tcp://localhost:5556", 1);

    // Start a background thread to check proxy connection
    //std::thread connection_checker(checkProxyConnection, std::ref(socket));

    bool connected_to_proxy = true;
    json request_json;
    int choice;

    while (true) {
        displayMenu();
        std::cin >> choice;

        switch (choice) {
            case 1: {
                // std::string base_url = "https://myshoppinglistapp.com/list/";
                // std::string list_id = generateUUID();
                // std::string full_url = base_url + list_id;

                // std::cout << "Give a name to your list: ";
                // std::string list_name;
                // std::cin >> list_name;

                // ShoppingList shoppingList(list_id, list_name, full_url);

                // ShoppingListResponse shoppingListResponse = addProductsToList(shoppingList);
                // std::cout << shoppingList.getItems() << std::endl;
                // json shoppingListJson = serializeGSet(shoppingList.getItems());

                if (connected_to_proxy) {
                    std::cout << "Create list - CLOUD MODE" << std::endl;
                    request_json["command"] = "CREATE_LIST";
                    // request_json["parameters"] = {{"list_url", full_url}, {"list_name", list_name}, {"list_items", shoppingListJson}};
                    //request_json["parameters"] = {{"list_url", full_url}, {"list_name", list_name}, {"list_items", "Work in progress"}};
                }
                else {
                    std::cout << "Create list - LOCAL MODE" << std::endl;
                    //saveListToLocal(db, shoppingList);
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
                    //request_json["parameters"] = {{"list_url", list_url}};;
                } 
                else {
                    std::cout << "Get list - LOCAL MODE" << std::endl;
                    // json list_data = loadListFromLocal(db, list_url);
                    // if (!list_data.empty()) {
                    //     std::cout << "Retrieved List: " << list_data.dump(4) << std::endl;
                    // } else {
                    //     std::cout << "List not found or unable to load." << std::endl;
                    // }
                }
                break;
            }
            case 3: {
                std::cout << "Thank You for using our platform!" <<std::endl;
                return 0;
            }
            default: {
                std::cout << "Invalid choice. Try again.\n\n";
                continue;
            }
        }

        if (choice == 1 || choice == 2) {
            std::string request = request_json.dump();

            if (choice == 1) {
                // Submits update made to list (PUSH)
                client.send_update(request);
            } else if (choice == 2) {
                // Ask for a list (DEALER)
                zmsg* msg = new zmsg(request.c_str());
                client.send("GET_LIST", msg);
                delete msg;
            }

            zmsg* reply = client.recv();
            if (reply) {
                std::string response(reinterpret_cast<const char*>(reply->pop_front().c_str()), reply->pop_front().size());
                std::cout << "Response from server: " << response << std::endl;
                delete reply;
            } else {
                std::cout << "No response received from the server." << std::endl;
            }
        }

        // Check for list updates via SUB socket
        if (choice != 3) {
            std::string sub_update = client.receive_update();
            if (!sub_update.empty()) {
                std::cout << "List update notification: " << sub_update << std::endl;
            }
        }

        // std::string request = request_json.dump();;
        // std::cout << request << std::endl;

        // zmq::message_t msg(request.size());
        // memcpy(msg.data(), request.c_str(), request.size());
        // socket.send(msg, zmq::send_flags::none);

        // zmq::message_t reply;
        // socket.recv(reply, zmq::recv_flags::none);

        // std::string response(static_cast<char*>(reply.data()), reply.size());
        // std::cout << "Response from server: " << response << std::endl;
    }

    return 0;
}
