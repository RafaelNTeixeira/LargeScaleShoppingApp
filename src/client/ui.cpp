#include <iostream>

void displayMenu() {
    std::cout << "______SHOPPING APPLICATION______\n";
    std::cout << "1. Create Shopping List\n";
    std::cout << "2. Load Shopping List\n";
    std::cout << "3. Update Shopping List\n";
    std::cout << "0. Exit\n";
    std::cout << "________________________________\n";
}

void listClientShoppingLists() {
    // ALTERAR PARA DAR DISPLAY ÀS SHOPPING LISTS QUE O CLIENTE TEM
    std::string url_list = "";
    for (int i = 0; i < 3; i++) {
        std::cout << "List " << i << std::endl;
    }
}

void listShoppingListItems(std::string url_list) {
    // ALTERAR PARA DAR DISPLAY AOS ITEMS DA SHOPPING LIST
    for (int i = 0; i < 3; i++) {
        std::cout << "Item " << i << std::endl;
    }
}