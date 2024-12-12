#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <zmq.hpp>

#include "../database.h"

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

Response handleRequest(std::string url_list, std::string request, zmsg* msg) {
    Response res;

    if (request == "CREATE_LIST") {
        ustring list_name = msg->pop_front();
        std::string list_name_str = (char*)list_name.c_str();
        std::cout << "List name received: " << list_name_str << std::endl;

        res.shopping_list = "[MOCK - CREATE_LIST] shopping list items: tomato - 1; potato - 2";
        res.reply = "create_list";
    } else if (request == "GET_LIST") {
        std::cout << "(NEEDS DEVELOPMENT) Got url for GET_LIST: " << url_list << std::endl;
        res.shopping_list = "[MOCK - GET_LIST] shopping list items: tomato - 1; potato - 2";
        res.reply = "get_list";
    } else if (request == "UPDATE_LIST") {
        ustring product_name = msg->pop_front();
        std::string product_name_str = (char*)product_name.c_str();
        std::cout << "Product name received: " << product_name_str << std::endl;

        ustring product_quant = msg->pop_front();
        std::string product_quantity_str = (char*)product_quant.c_str();
        int product_quantity = std::stoi(product_quantity_str);
        std::cout << "Product quantity received: " << product_quantity_str << std::endl;

        res.shopping_list = "[MOCK - GET_LIST] shopping list items: tomato - 1; potato - 2";
        res.reply = "update_list";
    } else {
        res.reply = "unknown request";
    }

    return res;
}