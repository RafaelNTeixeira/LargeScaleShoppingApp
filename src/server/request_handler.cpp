#include <iostream>
#include <string>
#include <zmq.hpp>
#include "database.cpp"

std::string handleRequest(const std::string request) {
    std::string response;

    std::cout << request;
    if (request == "CREATE_LIST") {
        createShoppingList("url", "name", "owner");
    } else if (request == "GET_LIST") {
        getShoppingList("url");
    } else {
        response = "Unknown Request";
    }

    std::cout << "Response: " << response << std::endl;

    return response;
}
