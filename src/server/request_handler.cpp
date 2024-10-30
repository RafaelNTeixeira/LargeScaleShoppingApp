#include <iostream>
#include <string>
#include <zmq.hpp>
#include "api.cpp"
//#include "data_types.h"

class RequestHandler {
public:
    void handleRequest(const std::string& request) {
        std::string response;

        if (request == "CREATE_LIST") {
            response = createShoppingList("Sample List"); 
        } else if (request == "GET_LIST") {
            response = getShoppingList("list1");
        } else {
            response = "Unknown Request";
        }

        std::cout << "Response: " << response << std::endl;

        return response;
    }
}