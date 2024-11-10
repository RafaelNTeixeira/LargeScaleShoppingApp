#include <iostream>
#include <string>
#include <zmq.hpp>
#include "database.cpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

json handleRequest(json& request) {
    json response_json;

    std::string command = request["command"];

    if (command == "CREATE_LIST") {
        std::string list_name = request["parameters"]["list_name"];
        std::string list_url = request["parameters"]["list_url"];
        //std::unordered_map<std::string, int> items = request["parameters"]["list_items"];

        createShoppingList(list_name);

        response_json["status"] = "success";
        response_json["message"] = "List created with ID " + list_url;
    } 
    else if (command == "GET_LIST") {
        std::string list_url = request["parameters"]["list_url"];

        getShoppingList(list_url);

        response_json["status"] = "success";
        response_json["message"] = "Retrieved list for URL: " + list_url;
    } 
    else if (command == "PING") {
        response_json["status"] = "success";
        response_json["message"] = "Connected to server";
    }
    else {
        response_json["status"] = "error";
        response_json["message"] = "Unknown Request";
    }

    return response_json;
}