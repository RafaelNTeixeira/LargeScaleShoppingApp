#include <uuid/uuid.h>

#include <atomic>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <zmq.hpp>

#include "../crdt/shopping_list.h"
#include "../database.h"
#include "majClient.cpp"
#include "ui.cpp"

using json = nlohmann::json;

// Method to print query results
static int callback(void* NotUsed, int argc, char** argv, char** azColName) {
    for (int i = 0; i < argc; i++) {
        std::cout << azColName[i] << ": " << argv[i] << std::endl;
    }
    return 0;
}

void saveListToLocal(Database& db, const ShoppingList& shoppingList) {
    json databaseShoppingList = db.get(shoppingList.getURL());

    if (!databaseShoppingList.empty()) {
        std::cout << "Shopping list already exists in the database. Updating..." << std::endl;
        ShoppingList databaseList;
        from_json(databaseShoppingList, databaseList);
        databaseList.join(shoppingList);
        json updatedShoppingListJson;
        to_json(updatedShoppingListJson, databaseList);
        db.set(shoppingList.getURL(), updatedShoppingListJson);
    } else {
        json shoppingListJson;
        to_json(shoppingListJson, shoppingList);
        db.set(shoppingList.getURL(), shoppingListJson);
    }
}

// json loadListFromLocal(sqlite3* db, const std::string& list_url) {
//     std::string sql = "SELECT * FROM shopping_lists WHERE url = '" + list_url + "';";
//     sqlite3_stmt* stmt;
//     json list_data;

//     int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
//     if (rc != SQLITE_OK) {
//         std::cerr << "Failed to fetch list: " << sqlite3_errmsg(db) << std::endl;
//         return list_data;
//     }

//     rc = sqlite3_step(stmt);
//     if (rc == SQLITE_ROW) {
//         list_data["url"] = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
//         list_data["name"] = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
//     } else {
//         std::cerr << "List not found." << std::endl;
//     }

//     sqlite3_finalize(stmt);

//     std::string sql_items = "SELECT item_name, acquired_quantity FROM shopping_lists_items WHERE list_url = '" + list_url + "';";
//     rc = sqlite3_prepare_v2(db, sql_items.c_str(), -1, &stmt, nullptr);
//     if (rc != SQLITE_OK) {
//         std::cerr << "Failed to fetch items: " << sqlite3_errmsg(db) << std::endl;
//         return list_data;
//     }

//     json items = json::array();
//     while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
//         json item;
//         item["item_name"] = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
//         item["acquired_quantity"] = sqlite3_column_int(stmt, 1);
//         items.push_back(item);
//     }

//     sqlite3_finalize(stmt);

//     list_data["items"] = items;

//     return list_data;
// }

// sqlite3* initializeDatabase() {
//     sqlite3* db;
//     int rc = sqlite3_open("database/local/shopping_lists.db", &db);

//     if (rc) {
//         std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
//         return nullptr;
//     }

//     const char* createTableSQL1 =
//         "CREATE TABLE IF NOT EXISTS shopping_lists ("
//         "url TEXT PRIMARY KEY, "
//         "name TEXT NOT NULL);";
//     executeSQL(db, createTableSQL1);

//     const char* createTableSQL2 =
//         "CREATE TABLE IF NOT EXISTS shopping_lists_items ("
//         "id INTEGER PRIMARY KEY AUTOINCREMENT, "
//         "list_url TEXT NOT NULL, "
//         "item_name TEXT NOT NULL, "
//         "acquired_quantity INTEGER DEFAULT 0, "
//         "FOREIGN KEY(list_url) REFERENCES shopping_lists(list_url));";
//     executeSQL(db, createTableSQL2);

//     return db;
// }

// Function to collect products from the user
bool addProductsToList(ShoppingList& shoppingList) {
    std::string product_name;
    int quantity;

    while (true) {
        std::cout << "Enter product name (or type 'q' to finish): ";
        std::cin >> product_name;

        if (product_name == "q") {
            break;
        }

        while (true) {
            std::cout << "Enter quantity for " << product_name << ": ";
            std::cin >> quantity;

            if (quantity <= 0) {
                std::cout << "Invalid quantity. Please enter a positive number." << std::endl;
                continue;
            }
            break;
        }

        if (!shoppingList.createItem(product_name, quantity)) {
            std::cout << "Product already exists in the list. Updating quantity..." << std::endl;
            shoppingList.increaseItem(product_name, quantity);
        }
        break;
    }

    return true;
}

bool buyProductsFromList(ShoppingList& shoppingList) {
    std::string product_name;
    int quantity;

    while (true) {
        std::cout << "Enter product name (or type 'q' to finish): ";
        std::cin >> product_name;
        if (product_name == "q") {
            break;
        }

        if (!shoppingList.contains(product_name)) {
            std::cout << "Product not found in the list. Please enter a valid product name." << std::endl;
            continue;
        }

        while (true) {
            std::cout << "Enter quantity to buy for " << product_name << ": ";
            std::cin >> quantity;

            if (quantity <= 0) {
                std::cout << "Invalid quantity. Please enter a positive number." << std::endl;
                continue;
            }
            break;
        }
        shoppingList.buyItem(product_name, quantity);
        break;
    }

    return true;
}

void updateList(Database& db, const std::string& list_url, json& shoppingList) {
    // make it thread safe
    if (db.exists(list_url)) {
        std::cout << "Shopping list already exists in the database. Updating..." << std::endl;
        json databaseShoppingList = db.get(list_url);
        ShoppingList databaseList;
        from_json(databaseShoppingList, databaseList);
        databaseList.join(shoppingList);
        json updatedShoppingListJson;
        to_json(updatedShoppingListJson, databaseList);
        db.set(list_url, updatedShoppingListJson);
    } else {
        std::cout << "Shopping list does not exist in the database. Creating..." << std::endl;
        shoppingList["id"] = mdcli::generateUUID();
        db.set(list_url, shoppingList);
    }
}

// Function receive shopping list updates from the SUB socket
void listenForUpdates(mdcli& client, Database& db) {
    while (s_interrupted == 0) {
        std::vector<std::string> sub_update = client.receive_updates();
        if (!sub_update.empty()) {
            std::string list_url = sub_update[0];
            try {
                json shopping_list = json::parse(sub_update[1].c_str());

                updateList(db, list_url, shopping_list);
            } catch (const json::parse_error& e) {
                std::cerr << "Error parsing JSON: " << e.what() << std::endl;
                std::cerr << "Received: " << sub_update[1] << std::endl;
            }
        }
    }
}

// Function receive heartbeats from the broker
void listenForHeartBeats(mdcli& client, Database& db) {
    bool last_cloud_mode = false;
    while (s_interrupted == 0) {
        zmsg* heartbeat = client.recv_heartbeat();
        if (last_cloud_mode != client.get_cloud_mode()) {
            // std::cout << "Cloud Mode updated: " << client.get_cloud_mode() << std::endl;
            last_cloud_mode = client.get_cloud_mode();
            if (client.get_cloud_mode()) {
                // std::cout << "Connected to the cloud." << std::endl;
                for (const auto& key : db.keys()) {
                    client.subscribe_to_list(key);
                }
            } else {
                // std::cout << "Disconnected from the cloud." << std::endl;
            }
        }
    }
}

int main(int argc, char* argv[]) {
    // usage : ./client <broker_ip/port> <path/to/database>
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <broker_ip/port> <broker_heartbeat_ip/port> <path/to/database>" << std::endl;
        return 1;
    }

    std::string broker_ip = argv[1];
    std::string broker_heartbeat_ip = argv[2];
    std::string db_path = argv[3];

    std::cout << "______SHOPPING APPLICATION______" << std::endl;
    std::cout << "Welcome to the Shopping Application!" << std::endl;

    std::cout << "Initialized local db..." << std::endl;
    Database db = Database();

    try {
        db.load(db_path);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    s_catch_signals();

    mdcli client(broker_ip, broker_heartbeat_ip, 0);

    std::thread update_listener(listenForUpdates, std::ref(client), std::ref(db));
    // update_listener.detach();  // Ensures the thread runs independently

    std::thread update_heartbeat(listenForHeartBeats, std::ref(client), std::ref(db));
    // update_heartbeat.detach();  // Ensures the thread runs independently

    json request_json;
    int choice;

    while (s_interrupted == 0) {
        displayMenu();
        std::cout << "Cloud Mode: " << client.get_cloud_mode() << std::endl;
        std::cout << "Enter choice: ";
        std::string get_list_url = "";
        std::string update_list_list_url = "";
        std::string list_name = "";
        ShoppingList currentShoppingList;

        json newShoppingListJson;
        json currentShoppingListJson;

        std::string base_url = "msla-";
        std::string list_id;
        std::string full_url;

        while (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid input. Please enter a valid choice: ";
        }

        switch (choice) {
            case 1: {
                list_id = mdcli::generateUUID();
                full_url = base_url + list_id;

                std::cout << "Give a name to your list: ";
                std::cin >> list_name;

                ShoppingList newShoppingList{list_id, list_name, full_url};
                to_json(newShoppingListJson, newShoppingList);
                db.set(full_url, newShoppingListJson);

                std::cout << "Shopping list created successfully!" << std::endl;
                std::cout << "List URL: " << full_url << std::endl;

                break;
            }
            case 2: {
                std::cout << "Enter List URL to retrieve: ";
                std::cin >> get_list_url;
                break;
            }
            case 3: {
                listClientShoppingLists(db);
                if (db.keys().empty()) {
                    break;
                }
                std::cout << "Pick shopping list url: ";
                std::cin >> update_list_list_url;
                if (!db.exists(update_list_list_url)) {
                    std::cout << "No shopping list found with that URL." << std::endl;
                    break;
                }
                currentShoppingListJson = db.get(update_list_list_url);
                if (currentShoppingListJson.empty()) {
                    std::cerr << "Error: Shopping list not found." << std::endl;
                    break;
                }
                from_json(currentShoppingListJson, currentShoppingList);
                int update_choice = -1;
                while (update_choice != 0) {
                    listShoppingListItems(currentShoppingList);
                    std::cout << "1. Add products to list" << std::endl;
                    std::cout << "2. Buy product" << std::endl;
                    std::cout << "0. Exit" << std::endl;
                    std::cout << "Enter choice: ";
                    std::cin >> update_choice;

                    switch (update_choice) {
                        case 1: {
                            addProductsToList(currentShoppingList);
                            break;
                        }
                        case 2: {
                            buyProductsFromList(currentShoppingList);
                            break;
                        }
                        case 0: {
                            break;
                        }
                        default: {
                            std::cout << "Invalid choice. Try again." << std::endl;
                            continue;
                        }
                    }
                }

                saveListToLocal(db, currentShoppingList);
                break;
            }
            case 4: {
                listClientShoppingLists(db);
                if (db.keys().empty()) {
                    break;
                }
                std::cout << "Enter List URL to delete: ";
                std::cin >> update_list_list_url;
                if (!db.exists(update_list_list_url)) {
                    std::cout << "No shopping list found with that URL." << std::endl;
                    break;
                }
                db.del(update_list_list_url);
                std::cout << "Shopping list deleted successfully!" << std::endl;
                break;
            }
            case 0: {
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
        if (choice != 0) {
            if (choice == 1) {
                // Create an empty Shopping List (DEALER)
                // std::cout << "cloud_mode: " << cloud_mode << std::endl;
                if (cloud_mode) {
                    zmsg* msg = new zmsg();
                    std::string shoppingListStr = newShoppingListJson.dump();
                    // std::cout << "List: " << shoppingListStr << std::endl;
                    msg->push_front(shoppingListStr.c_str());
                    msg->push_front(list_name.c_str());
                    msg->push_front(full_url.c_str());
                    // std::cout << "MSG SENT TO HANDLE op 1: " << std::endl;
                    client.send("LIST_MANAGEMENT", "CREATE_LIST", msg);
                    delete msg;
                    client.subscribe_to_list(full_url);
                }
            } else if (choice == 2) {
                // Ask for a Shopping List (DEALER)
                // std::cout << "cloud_mode: " << cloud_mode << std::endl;
                const char* url_list_msg_parameter = get_list_url.c_str();
                if (cloud_mode) {
                    client.subscribe_to_list(get_list_url);
                    zmsg* msg = new zmsg(url_list_msg_parameter);
                    client.send("LIST_MANAGEMENT", "GET_LIST", msg);
                } else {
                    std::cout << "xxxxxxxxx Server Unavailable. Try again later! xxxxxxxxx" << std::endl;
                }
            } else if (choice == 3) {
                // std::cout << "cloud_mode: " << cloud_mode << std::endl;
                if (cloud_mode && db.exists(update_list_list_url)) {
                    zmsg* msg = new zmsg();
                    to_json(currentShoppingListJson, currentShoppingList);
                    std::string shoppingListStr = currentShoppingListJson.dump();

                    msg->push_front(shoppingListStr.c_str());
                    msg->push_front(update_list_list_url.c_str());
                    // std::cout << "MSG SENT TO HANDLE op 3: " << std::endl;
                    client.send("LIST_MANAGEMENT", "UPDATE_LIST", msg);
                    client.subscribe_to_list(update_list_list_url);
                    delete msg;
                }
            } else if (choice == 4) {
                // std::cout << "cloud_mode: " << cloud_mode << std::endl;
                if (cloud_mode) {
                    client.unsubscribe_from_list(update_list_list_url);
                    // std::cout << "MSG SENT TO HANDLE op 4: " << std::endl;
                }
            }

            if (cloud_mode == 1) {
                zmsg* reply = client.recv();
                if (reply) {
                    std::cout << "Reply received: " << std::endl;
                    // reply->dump();

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

    for (const auto& key : db.keys()) {
        client.unsubscribe_from_list(key);
    }

    // End the other threads
    update_listener.join();
    update_heartbeat.join();

    try {
        db.save(db_path);
        std::cout << "Database saved successfully." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
