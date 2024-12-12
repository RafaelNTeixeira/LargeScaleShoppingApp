#include <iostream>

void displayMenu() {
    std::cout << "______SHOPPING APPLICATION______\n";
    std::cout << "1. Create Shopping List\n";
    std::cout << "2. Load Shopping List\n";
    std::cout << "3. Update Shopping List\n";
    std::cout << "4. Delete Shopping List\n";
    std::cout << "0. Exit\n";
    std::cout << "________________________________\n";
}

void listClientShoppingLists(Database& db) {
    std::vector<std::string> keys = db.keys();
    if (keys.empty()) {
        std::cout << "No shopping lists found." << std::endl;
        return;
    }
    for (const auto& key : keys) {
        std::cout << key << " - " << db.get(key)["title"] << std::endl;
    }
}

void listShoppingListItems(ShoppingList& shoppingList) {
    std::set<Item> items = shoppingList.getItemsSet();
    if (items.empty()) {
        std::cout << "No items found in the shopping list." << std::endl;
        return;
    }
    for (const auto& item : items) {
        std::cout << item.first << " - " << item.second << std::endl;
    }
}