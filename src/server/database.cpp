#include <iostream>
#include <string>
#include <unordered_map>
#include <sqlite3.h> 
#include "../database/database.sql"

sqlite3* db;

bool initDatabase() {
    if (sqlite3_open("database.db", &db) != SQLITE_OK) {
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

void createShoppingList(const std::string& url, const std::string& name, const std::string& owner) {
    std::string sql = "INSERT INTO shopping_list (url, name, owner) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    sqlite3_bind_text(stmt, 1, url.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, owner.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Failed to create list: " << sqlite3_errmsg(db) << std::endl;
    } else {
        std::cout << "Created shopping list: " << name << std::endl;
    }

    sqlite3_finalize(stmt);
}

void createShoppingListItem(const std::string& list_url, const std::string& item_name, int target_quantity, const std::string& added_by) {
    std::string sql = "INSERT INTO shopping_list_items (list_url, item_name, target_quantity, added_by) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    sqlite3_bind_text(stmt, 1, list_url.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, item_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, target_quantity);
    sqlite3_bind_text(stmt, 4, added_by.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Failed to add item to list: " << sqlite3_errmsg(db) << std::endl;
    } else {
        std::cout << "Added item: " << item_name << " to list: " << list_url << std::endl;
    }

    sqlite3_finalize(stmt);
}

void getShoppingList(const std::string& url) {
    std::string sql = "SELECT url, name, owner, created_at, updated_at, deleted FROM shopping_list WHERE url = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    sqlite3_bind_text(stmt, 1, url.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        std::cout << "List URL: " << sqlite3_column_text(stmt, 0)
                  << "\nName: " << sqlite3_column_text(stmt, 1)
                  << "\nOwner: " << sqlite3_column_text(stmt, 2)
                  << "\nCreated At: " << sqlite3_column_text(stmt, 3)
                  << "\nUpdated At: " << sqlite3_column_text(stmt, 4)
                  << "\nDeleted: " << sqlite3_column_int(stmt, 5) << std::endl;
    } else {
        std::cout << "No list found with URL: " << url << std::endl;
    }

    sqlite3_finalize(stmt);
}

void getShoppingListItems(const std::string& list_url) {
    std::string sql = "SELECT id, item_name, target_quantity, acquired_quantity, added_by FROM shopping_list_items WHERE list_url = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    sqlite3_bind_text(stmt, 1, list_url.c_str(), -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::cout << "Item ID: " << sqlite3_column_int(stmt, 0)
                  << "\nItem Name: " << sqlite3_column_text(stmt, 1)
                  << "\nTarget Quantity: " << sqlite3_column_int(stmt, 2)
                  << "\nAcquired Quantity: " << sqlite3_column_int(stmt, 3)
                  << "\nAdded By: " << sqlite3_column_text(stmt, 4) << "\n" << std::endl;
    }

    sqlite3_finalize(stmt);
}

