#include <iostream>
#include <string>
#include <zmq.hpp>
//#include "api.cpp"
//#include "data_types.h"

class RequestHandler {
public:
    void handleRequest(const std::string& request) {
        std::string response;

        if (request == "CREATE_LIST") {
            response = createList("Sample List"); // CHANGE TO THE FUNCTION IN createShoppingList() IN server/api.cpp
        } else if (request == "GET_LIST") {
            response = getList("list1");  // CHANGE TO THE FUNCTION IN getShoppingList() IN server/api.cpp
        } else {
            response = "Unknown Request";
        }

        std::cout << "Response: " << response << std::endl;
    }

private:
    // TEMP. REMOVE AFTER INSERTING IN handleRequest() the api functions
    std::string createList(const std::string& name) {
        return "Created list: " + name;
    }

    // TEMP. REMOVE AFTER INSERTING IN handleRequest() the api functions
    std::string getList(const std::string& id) {
        return "Retrieved list: " + id;
    }
};