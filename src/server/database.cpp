#include <iostream>
#include <string>
#include <unordered_map>
#include <sqlite3.h> 
#include <random>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

sqlite3* db;

bool initDatabase() {
    if (sqlite3_open("database/cloud/database.db", &db) != SQLITE_OK) {
        std::cerr << "Failed to open the database: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    return true;
}

void closeDatabase() {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
}

std::string generateUniqueURL(const std::string& listName) {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<int> distribution(100000000, 999999999);
    int random_number = distribution(generator);

    return listName + "/" + std::to_string(random_number);
}

void createShoppingListItem(const std::string& list_url, const std::string& item_name, const std::string& added_by, int total_added, int total_deleted) {
    std::string sql = "INSERT INTO shopping_list_items (list_url, item_name, added_by, total_added, total_deleted) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    sqlite3_bind_text(stmt, 1, list_url.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, item_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, added_by.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, total_added);
    sqlite3_bind_int(stmt, 5, total_deleted);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Failed to add item to list: " << sqlite3_errmsg(db) << std::endl;
    } else {
        std::cout << "Added item: " << item_name << " to list: " << list_url << std::endl;
    }

    sqlite3_finalize(stmt);
}

void createShoppingList(const std::string& name, std::unordered_map<std::string, int>& items) {
    std::string url = generateUniqueURL(name); 
    std::string sql = "INSERT INTO shopping_list (url, name) VALUES (?, ?);";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    sqlite3_bind_text(stmt, 1, url.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Failed to create list: " << sqlite3_errmsg(db) << std::endl;
    } else {
        std::cout << "Created shopping list with URL: " << url << std::endl;
    }

    sqlite3_finalize(stmt);

    for (const auto item : items) {
        createShoppingListItem(url, item.first, "", item.second, 0);
    }
}

json getShoppingList(const std::string& url) {
    std::string sql = "SELECT url, name, created_at, updated_at FROM shopping_list WHERE url = ?;";
    sqlite3_stmt* stmt;
    json response_json;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        response_json["status"] = "error";
        response_json["message"] = "Failed to fetch shopping list.";
        return response_json;
    }

    sqlite3_bind_text(stmt, 1, url.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        response_json["status"] = "success";
        response_json["url"] = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        response_json["name"] = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        response_json["created_at"] = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        response_json["updated_at"] = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
        response_json["data"] = {
            {"url", response_json["url"]},
            {"name", response_json["name"]},
            {"created_at", response_json["created_at"]},
            {"updated_at", response_json["updated_at"]}
        };
    } else {
        response_json["status"] = "error";
        response_json["message"] = "No list found with URL: " + url;
    }

    sqlite3_finalize(stmt);
    return response_json;
}

json getShoppingListItems(const std::string& list_url) {
    std::string sql = "SELECT id, item_name, acquired_quantity, added_by, total_added, total_deleted FROM shopping_list_items WHERE list_url = ?;";
    sqlite3_stmt* stmt;
    json response_json;
    json items_array = json::array();

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        response_json["status"] = "error";
        response_json["message"] = "Failed to fetch shopping list items.";
        return response_json;
    }

    sqlite3_bind_text(stmt, 1, list_url.c_str(), -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        json item;
        item["id"] = sqlite3_column_int(stmt, 0);
        item["item_name"] = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        item["acquired_quantity"] = sqlite3_column_int(stmt, 2);
        item["added_by"] = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
        item["total_added"] = sqlite3_column_int(stmt, 4);
        item["total_deleted"] = sqlite3_column_int(stmt, 5);
        items_array.push_back(item);
    }

    if (items_array.empty()) {
        response_json["status"] = "error";
        response_json["message"] = "No items found for list URL: " + list_url;
    } else {
        response_json["status"] = "success";
        response_json["items"] = items_array;
    }

    sqlite3_finalize(stmt);
    return response_json;
}

void updateItemAcquiredQuantity(int item_id, int new_acquired_quantity) {
    std::string sql = "UPDATE shopping_list_items SET acquired_quantity = ? WHERE id = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    sqlite3_bind_int(stmt, 1, new_acquired_quantity);
    sqlite3_bind_int(stmt, 2, item_id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Failed to update acquired quantity: " << sqlite3_errmsg(db) << std::endl;
    } else {
        std::cout << "Updated acquired quantity for item ID: " << item_id << std::endl;
    }

    sqlite3_finalize(stmt);
}

void updateItemName(int item_id, const std::string& new_item_name) {
    std::string sql = "UPDATE shopping_list_items SET item_name = ? WHERE id = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    sqlite3_bind_text(stmt, 1, new_item_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, item_id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Failed to update item name: " << sqlite3_errmsg(db) << std::endl;
    } else {
        std::cout << "Updated item name for item ID: " << item_id << std::endl;
    }

    sqlite3_finalize(stmt);
}

void updateShoppingListName(const std::string& url, const std::string& newName) {
    std::string sql = "UPDATE shopping_list SET name = ?, updated_at = CURRENT_TIMESTAMP WHERE url = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    sqlite3_bind_text(stmt, 1, newName.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, url.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Failed to update list name: " << sqlite3_errmsg(db) << std::endl;
    } else {
        std::cout << "Updated shopping list name to: " << newName << std::endl;
    }

    sqlite3_finalize(stmt);
}

void incrementTotalAdded(int item_id, int amount) {
    std::string sql = "UPDATE shopping_list_items SET total_added = total_added + ? WHERE id = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    sqlite3_bind_int(stmt, 1, amount);
    sqlite3_bind_int(stmt, 2, item_id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Failed to increment total added: " << sqlite3_errmsg(db) << std::endl;
    } else {
        std::cout << "Incremented total added by " << amount << " for item ID: " << item_id << std::endl;
    }

    sqlite3_finalize(stmt);
}

void incrementTotalDeleted(int item_id, int amount) {
    std::string sql = "UPDATE shopping_list_items SET total_deleted = total_deleted + ? WHERE id = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    sqlite3_bind_int(stmt, 1, amount);
    sqlite3_bind_int(stmt, 2, item_id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Failed to increment total deleted: " << sqlite3_errmsg(db) << std::endl;
    } else {
        std::cout << "Incremented total deleted by " << amount << " for item ID: " << item_id << std::endl;
    }

    sqlite3_finalize(stmt);
}

int calculateTargetQuantity(int item_id) {
    std::string sql = "SELECT total_added - total_deleted AS target_quantity FROM shopping_list_items WHERE id = ?;";
    sqlite3_stmt* stmt;
    int target_quantity = 0;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return target_quantity;
    }

    sqlite3_bind_int(stmt, 1, item_id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        target_quantity = sqlite3_column_int(stmt, 0);
        std::cout << "Target quantity for item ID " << item_id << " is " << target_quantity << std::endl;
    } else {
        std::cerr << "No item found with ID: " << item_id << std::endl;
    }

    sqlite3_finalize(stmt);
    return target_quantity;
}