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

json getJsonFromUString(const zmsg::ustring& ustr) {
    return json::parse((char*)ustr.c_str());
}

Response handleRequest(std::string url_list, std::string request, zmsg* msg, Database& db) {
    Response res;

    if (request == "CREATE_LIST") {
        ustring list_name = msg->pop_front();

        std::string list_name_str = (char*)list_name.c_str();

        // THIS IS GONNA NEED CHECKING TO CONVERT
        zmsg::ustring shopping_list_str = msg->pop_front();
        if (shopping_list_str.size() == 0) {
            std::cout << "Empty shopping list received" << std::endl;
        }
        json shopping_list = getJsonFromUString(shopping_list_str);
        std::cout << "Shopping list received: " << shopping_list << std::endl;
        ShoppingList new_list;
        from_json(shopping_list, new_list);
        new_list.setId("");  // ID is not stored in the database
        to_json(shopping_list, new_list);

        db.set(new_list.getURL(), shopping_list);

        res.shopping_list = shopping_list;
        res.reply = "create_list";
    } else if (request == "GET_LIST") {
        std::cout << "Got url for GET_LIST: " << url_list << std::endl;
        json shopping_list = db.get(url_list);

        res.shopping_list = shopping_list;
        res.reply = "get_list";
    } else if (request == "UPDATE_LIST") {
        zmsg::ustring received_shopping_list_str = msg->pop_front();
        json received_shopping_list = getJsonFromUString(received_shopping_list_str);
        json stored_shopping_list = db.get(url_list);
        ShoppingList new_list;
        ShoppingList stored_list;

        from_json(received_shopping_list, new_list);
        from_json(stored_shopping_list, stored_list);

        stored_list.join(new_list);
        to_json(stored_shopping_list, stored_list);

        db.set(url_list, stored_shopping_list);

        res.shopping_list = stored_shopping_list;
        res.reply = "update_list";
    } else {
        res.reply = "unknown request";
    }

    return res;
}

#endif