#include <iostream>
#include "../data_types.h"

void displayMenu() {
    std::cout << "______SHOPPING APPLICATION______\n";
    std::cout << "1. Create Shopping List\n";
    std::cout << "2. Load Shopping List\n";
    std::cout << "3. Exit\n";
    std::cout << "________________________________\n";
}

void createShoppingList() {
    ShoppingList list;

    // TEMP
    list.user_id = "user123";
    list.name = "Grocery List";
    list.created_at = std::time(nullptr);
    list.updated_at = list.created_at;
    list.items.push_back({"item1", "Milk", 2});
    list.items.push_back({"item2", "Bread", 1});

    // NEED TO SAVE THE SHOPPING LIST LOCALLY HERE

    std::cout << "Shopping list created and saved.\n";
}
