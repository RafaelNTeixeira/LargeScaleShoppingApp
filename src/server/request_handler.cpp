#include <iostream>
#include <string>
#include <zmq.hpp>
#include "database.cpp"

std::string handleRequest(const std::string& request) {
    std::string response;

    size_t pos = request.find(':');
    std::string command = request.substr(0, pos);
    std::string arguments = request.substr(pos + 1);

    if (command == "CREATE_LIST") {
        size_t separator_pos = arguments.find(':');
        std::string list_name = arguments.substr(0, separator_pos);
        std::string owner = arguments.substr(separator_pos + 1);

        createShoppingList(list_name);
        response = "Shopping list '" + list_name + "'.";
    } else if (command == "GET_LIST") {
        std::string url = arguments;
        getShoppingList(url);
        response = "Retrieved list for URL: " + url;
    } else {
        response = "Unknown Request";
    }

    return response;
}