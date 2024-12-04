#include <iostream>
#include <string>
#include <zmq.hpp>
#include "database.cpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct Response {
    std::string reply;
    std::string shopping_list;
};

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

Response handleRequest(std::string request) {
    Response res;

    if (request == "CREATE_LIST") {
        res.shopping_list = "[MOCK - CREATE_LIST] shopping list items: tomato - 1; potato - 2";
        res.reply = "create_list";
    } 
    else if (request == "GET_LIST") {
        res.shopping_list = "[MOCK - GET_LIST] shopping list items: tomato - 1; potato - 2";
        res.reply = "get_list";
    } 
    else {
       res.reply = "unknown request";
    }

    return res;
}