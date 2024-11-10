#include <iostream>
#include <string>
#include <zmq.hpp>
#include "database.cpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

json concatenateResponses(const json& response_json1, const json& response_json2) {
    json response_json;

    if (response_json1["status"] == "error") {
        response_json["status"] = "error";
        response_json["message"] = response_json1["message"];
    } else if (response_json2["status"] == "error") {
        response_json["status"] = "error";
        response_json["message"] = response_json2["message"];
    } else {
        response_json["status"] = "success";
        
        response_json["shopping_list"] = response_json1["data"];

        if (response_json2.contains("items")) {
            response_json["items"] = response_json2["items"];
        } else {
            response_json["items"] = {};
        }
    }

    return response_json;
}

json handleRequest(json& request) {
    json response_json;

    if (!request.contains("command")) {
        response_json["status"] = "error";
        response_json["message"] = "Missing 'command' in request.";
        return response_json;
    }

    std::string command = request["command"];

    if (command == "CREATE_LIST") {
        std::string list_name = request["parameters"]["list_name"];
        std::string list_url = request["parameters"]["list_url"];
        std::unordered_map<std::string, int> items = request["parameters"]["list_items"];

        createShoppingList(list_name, items);

        response_json["status"] = "success";
        //response_json["message"] = "List created with ID " + list_url;
    } 
    else if (command == "GET_LIST") {
        std::string list_url = request["parameters"]["list_url"];

        json response_json1 = getShoppingList(list_url);
        json response_json2 = getShoppingListItems(list_url);

        response_json = concatenateResponses(response_json1, response_json2);
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