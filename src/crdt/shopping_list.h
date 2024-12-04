#ifndef NEW_SHOPPING_LIST_H
#define NEW_SHOPPING_LIST_H

#include <nlohmann/json.hpp>
#include <set>
#include <unordered_map>

#include "counter.h"
#include "join.h"
#include "ormap.h"

class ShoppingList;

typedef std::pair<std::string, int> Item;

class ShoppingList {
   private:
    std::string id;
    std::string title;
    std::string url;
    ORMap<std::string, CausalCounter<int>> items;

   public:
    ShoppingList() {}
    ShoppingList(const std::string& id, const std::string& title, const std::string& url) : id(id), title(title), url(url) { items = ORMap<std::string, CausalCounter<int>>(); }
    ShoppingList(const ShoppingList& o) : id(o.id), title(o.title), url(url), items(o.items) {}

    std::string getId() const {
        return id;
    }

    std::string getTitle() const {
        return title;
    }

    std::string getURL() const {
        return url;
    }

    ORMap<std::string, CausalCounter<int>> getItems() const {
        return items;
    }

    std::set<Item> getItemsSet() const {
        std::set<Item> res;
        for (const auto& item : items.map) {
            int read = item.second.read();
            if (read > 0) {
                res.insert({item.first, read});
            }
        }
        return res;
    }

    bool contains(const std::string& name) const {
        for (const auto& item : items.map) {
            if (item.first == name) {
                return item.second.read() > 0;
            }
        }
        return false;
    }

    int getQuantity(const std::string& name) const {
        for (const auto& item : items.map) {
            if (item.first == name) {
                return item.second.read();
            }
        }
        return 0;
    }

    bool createItem(const std::string& name, int quantity = 1) {
        bool res;
        if (contains(name)) {
            res = false;
        } else {
            res = true;
            items[name].inc(id, quantity);
        }
        return res;
    }

    bool removeItem(const std::string& name) {
        bool res;
        if (!contains(name)) {
            res = false;
        } else {
            items.remove(name);
            res = true;
        }
        return res;
    }

    bool increaseItem(const std::string& name, int quantity = 1) {
        bool res;
        if (!contains(name)) {
            res = false;
        } else {
            items[name].inc(id, quantity);
            res = true;
        }
        return res;
    }

    bool buyItem(const std::string& name, int quantity = 1) {
        bool res;
        if (!contains(name)) {
            res = false;
        } else {
            items[name].dec(id, quantity);
            res = true;
        }
        return res;
    }

    bool join(const ShoppingList& o) {
        if (title != o.title || url != o.url) {
            return false;
        }
        items.join(o.items);
        return true;
    }

    bool operator==(const ShoppingList& o) const {
        return id == o.id && title == o.title && url == o.url && items == o.items;
    }

    bool operator!=(const ShoppingList& o) const {
        return !(*this == o);
    }

    Item getItem(const std::string& name) const {
        for (const auto& item : items.map) {
            if (item.first == name) {
                return {item.first, item.second.read()};
            }
        }
        return {name, 0};
    }
    friend void to_json(nlohmann::json& j, const ShoppingList& u);
    friend void from_json(const nlohmann::json& j, ShoppingList& u);
};

void to_json(nlohmann::json& j, const ShoppingList& u) {
    nlohmann::json items;
    items["context"] = u.items.context.toJson();
    for (const auto& item : u.items.map) {
        items["map"][item.first] = item.second.core.values;
    }
    j = nlohmann::json{{"id", u.getId()}, {"title", u.getTitle()}, {"url", u.getURL()}, {"items", items}};
}

void from_json(const nlohmann::json& j, ShoppingList& u) {
    u.id = j.at("id").get<std::string>();
    u.title = j.at("title").get<std::string>();
    u.url = j.at("url").get<std::string>();

    ShoppingList temp{u.id, u.title, u.url};
    temp.items.base.fromJson(j.at("items").at("context"));
    for (const auto& item : j.at("items").at("map").items()) {
        CausalCounter<int> counter;
        counter.core.values = item.value();
        temp.items.map[item.key()] = counter;
    }
    u.join(temp);
}

#endif
