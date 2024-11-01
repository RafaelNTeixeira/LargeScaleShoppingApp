#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include <ctime>
#include <string>
#include <vector>

struct Item {
    std::string id;
    std::string name;
    int quantity;
};

struct ShoppingList {
    std::string id;
    std::string user_id;     // Owner of the list
    std::string name;        // Name of the shopping list
    std::time_t created_at;  // Timestamp for creation
    std::time_t updated_at;  // Timestamp for the last update
    std::vector<Item> items;
};

#endif  // DATA_TYPES_H