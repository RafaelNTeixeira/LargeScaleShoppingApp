#ifndef NEW_SHOPPING_LIST_H
#define NEW_SHOPPING_LIST_H

#include <set>
#include <unordered_map>

#include "counter.h"
#include "join.h"
#include "new_aworset.h"

// class Item {
//    public:
//     std::string name;
//     Counter<int> quantity;

//     Item() = default;
//     Item(const std::string& name) : name(name) {}
//     Item(const std::string& name, const Counter<int>& quantity) : name(name), quantity(quantity) {}

//     bool operator==(const Item& o) const {
//         return name == o.name;
//     }

//     bool operator<(const Item& o) const {
//         return name < o.name;
//     }

//     bool operator!=(const Item& o) const {
//         return name != o.name || !(quantity == o.quantity);
//     }

//     void join(const Item& o) {
//         quantity.join(o.quantity);
//     }

//     Item inc(std::string replica_id, int value) {
//         Item res;
//         res.name = name;
//         res.quantity = quantity.inc(replica_id, value);
//         return res;
//     }

//     Item dec(std::string replica_id, int value) {
//         Item res;
//         res.name = name;
//         res.quantity = quantity.dec(replica_id, value);
//         return res;
//     }

//     int local(std::string replica_id) const {
//         return quantity.local(replica_id);
//     }

//     int read() const {
//         return quantity.read();
//     }
// };

// class ShoppingList;

// typedef std::pair<bool, ShoppingList> ShoppingListResponse;

// class ShoppingList {
//    private:
//     std::string id;
//     std::string title;
//     std::string url;
//     AWORSet<Item> items;

//    public:
//     ShoppingList() {}
//     ShoppingList(const std::string& id, const std::string& title, const std::string& url) : id(id), title(title), url(url) { items = AWORSet<Item>(); }
//     ShoppingList(const ShoppingList& o) : id(o.id), title(o.title), url(url), items(o.items) {}

//     std::string getTitle() const {
//         return title;
//     }

//     std::string getURL() const {
//         return url;
//     }

//     std::set<Item> getItems() const {
//         return items.allValues();
//     }

//     bool contains(const std::string& name) const {
//         for (const auto& item : items.allValues()) {
//             if (item.name == name) {
//                 return true;
//             }
//         }
//         return false;
//     }

//     int getQuantity(const std::string& name) const {
//         for (const auto& item : items.allValues()) {
//             if (item.name == name) {
//                 return item.read();
//             }
//         }
//         return 0;
//     }

//     ShoppingListResponse createItem(const std::string& name, int quantity = 1) {
//         AWORSet<Item> item_delta;
//         bool res;
//         if (contains(name)) {
//             res = false;
//             item_delta = items;
//         } else {
//             res = true;
//             Item item(name);
//             item_delta = items.add(name, item.inc(id, quantity));
//         }
//         ShoppingList sl(id, title, url);
//         sl.items = item_delta;
//         return {res, sl};
//     }

//     ShoppingListResponse removeItem(const std::string& name) {
//         AWORSet<Item> item_delta;
//         bool res;
//         if (!contains(name)) {
//             res = false;
//             item_delta = items;
//         } else {
//             res = true;
//             item_delta = items.remove(name);
//         }
//         ShoppingList sl(id, title, url);
//         sl.items = item_delta;
//         return {res, sl};
//     }

//     ShoppingListResponse increaseItem(const std::string& name, int quantity = 1) {
//         AWORSet<Item> item_delta;
//         bool res;
//         if (!contains(name)) {
//             res = false;
//             item_delta = items;
//         } else {
//             res = true;
//             Item item(name);
//             item_delta = items.add(name, item.inc(id, quantity));
//         }
//         ShoppingList sl(id, title, url);
//         sl.items = item_delta;
//         return {res, sl};
//     }

//     ShoppingListResponse buyItem(const std::string& name, int quantity = 1) {
//         AWORSet<Item> item_delta;
//         bool res;
//         if (!contains(name)) {
//             res = false;
//             item_delta = items;
//         } else {
//             res = true;
//             Item item(name);
//             item_delta = items.add(name, item.dec(id, quantity));
//         }
//         ShoppingList sl(id, title, url);
//         sl.items = item_delta;
//         return {res, sl};
//     }

//     ShoppingListResponse updateList(const ShoppingList& other) {
//         AWORSet<Item> item_delta;
//         item_delta.join(items);
//         item_delta.join(other.items);
//         ShoppingList sl(id, title, url);
//         sl.items = item_delta;
//         return {true, sl};
//     }

//     void join(const ShoppingList& o) {
//         items.join(o.items);
//     }

//     bool operator==(const ShoppingList& o) const {
//         return id == o.id && title == o.title && url == o.url && items == o.items;
//     }

//     bool operator!=(const ShoppingList& o) const {
//         return !(*this == o);
//     }

//     std::pair<bool, Item> getItem(const std::string& name) const {
//         for (const auto& item : items.allValues()) {
//             if (item.name == name) {
//                 return {true, item};
//             }
//         }
//         return {false, Item()};
//     }
// };

// #endif

class ShoppingList;

typedef std::pair<std::string, int> Item;

class ShoppingList {
   private:
    std::string id;
    std::string title;
    std::string url;
    ORMap<std::string, CausalCounter<int>> items;

   public:
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
};

#endif
