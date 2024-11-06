#include <iostream>
#include <string>
#include <unordered_map>
#include "../data_types.h"
#include "../database/database.sql"

std::unordered_map<std::string, ShoppingList> database;

void createShoppingList(const ShoppingList& list) {
    database[list.id] = list;
    std::cout << "Created shopping list: " << list.name << std::endl;
}

ShoppingList getShoppingList(const std::string id) {
    return database[id];
}


