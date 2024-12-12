#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <zmq.hpp>

#include "../crdt/shopping_list.h"
#include "../database.h"

using json = nlohmann::json;

struct Response {
    std::string reply;
    json shopping_list;
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

Response handleRequest(std::string url_list, std::string request, zmsg* msg, Database& db) {
    Response res;

    if (request == "CREATE_LIST") {
        ustring list_name = msg->pop_front();

        std::string list_name_str = (char*)list_name.c_str();
        std::string generated_url = (char*)msg->pop_front().c_str();

        std::cout << "List name received: " << list_name_str << std::endl;

        // THIS IS GONNA NEED CHECKING TO CONVERT
        json shopping_list = msg->pop_front();
        ShoppingList new_list;
        from_json(shopping_list, new_list);
        new_list.setId("");  // ID is not stored in the database
        to_json(shopping_list, new_list);

        db.set(generated_url, shopping_list);

        res.shopping_list = shopping_list;
        res.reply = "create_list";
    } else if (request == "GET_LIST") {
        std::cout << "Got url for GET_LIST: " << url_list << std::endl;
        json shopping_list = db.get(url_list);

        res.shopping_list = shopping_list;
        res.reply = "get_list";
    } else if (request == "UPDATE_LIST") {
        std::string url = (char*)msg->pop_front().c_str();

        std::cout << "List URL received: " << url << std::endl;

        json received_shopping_list = msg->pop_front();
        json stored_shopping_list = db.get(url);
        ShoppingList new_list;
        ShoppingList stored_list;

        from_json(received_shopping_list, new_list);
        from_json(stored_shopping_list, stored_list);

        stored_list.join(new_list);
        to_json(stored_shopping_list, stored_list);

        db.set(url, stored_shopping_list);

        res.shopping_list = stored_shopping_list;
        res.reply = "update_list";
    } else {
        res.reply = "unknown request";
    }

    return res;
}

#endif