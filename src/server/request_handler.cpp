#include <iostream>
#include <string>
#include <zmq.hpp>
#include "database.cpp"
//#include "data_types.h"

class RequestHandler {
public:
    std::string handleRequest(const std::string& request) {
        ShoppingList shoppingList;
        std::string response;

        if (request == "CREATE_LIST") {
            // Use function createShoppingList() from database.cpp
        } else if (request == "GET_LIST") {
            // Use function getShoppingList() from database.cpp
        } else {
            response = "Unknown Request";
        }

        std::cout << "Response: " << response << std::endl;

        return response;
    };
};