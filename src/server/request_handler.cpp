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
        response_json["items"] = response_json2.contains("items") ? response_json2["items"] : json::array();
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
        
        response_json["status"] = "success";
        response_json["message"] = "List created successfully.";
    } 
    else if (command == "GET_LIST") {
        response_json["status"] = "error";
        response_json["message"] = "Missing 'list_url' for GET_LIST request.";
    } 
    else {
        response_json["status"] = "error";
        response_json["message"] = "Unknown Request";
    }

    return response_json;
}