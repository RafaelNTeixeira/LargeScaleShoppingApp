#ifndef SHOPPING_LIST_H
#define SHOPPING_LIST_H

#include <vector>

#include "gset.h"

enum ActionType {
    CREATE,
    REMOVE,
    INCREASE,
    BUY
};

struct Action {
    ActionType type;
    int quantity;
    std::string target;

    bool operator==(const Action& other) const {
        return (type == other.type) &&
               (quantity == other.quantity) &&
               (target == other.target);
    }
};

typedef std::pair<bool, std::pair<GSet<std::string>, GSet<std::string>>> ShoppingListResponse;

class ShoppingList {
   private:
    std::string id;
    std::string title;
    std::string url;
    GSet<std::string> items;
    GSet<std::string> items_acquired;
    std::vector<Action> history;

    Action addAction(ActionType type, const std::string& target, int quantity = 0) {
        Action a;
        a.type = type;
        a.target = target;
        a.quantity = quantity;
        history.push_back(a);
        return a;
    }

   public:
    ShoppingList() {}
    ShoppingList(const std::string& id, const std::string& title, const std::string& url) : id(id), title(title), url(url) { items = GSet<std::string>(id); }
    ShoppingList(const ShoppingList& o) : id(o.id), title(o.title), url(url), items(o.items), items_acquired(o.items_acquired), history(o.history) {}

    std::string getTitle() const {
        return title;
    }

    std::string getURL() const {
        return url;
    }

    GSet<std::string> getItems() const {
        return items;
    }

    GSet<std::string> getItemsAquired() const {
        return items_acquired;
    }

    bool contains(const std::string& name) const {
        return items.read(name) > 0;
    }

    int getQuantity(const std::string& name) const {
        return items.read(name);
    }

    int getQuantityAcquired(const std::string& name) const {
        return items_acquired.read(name);
    }

    std::set<std::string> elements() {
        std::set<std::string> res;
        for (const auto& i : items.elements()) {
            if (items.read(i) > 0) {
                res.insert(i);
            }
        }
        return res;
    }

    std::unordered_map<std::string, int> elementsRead() {
        std::unordered_map<std::string, int> res;
        for (const auto& i : items.elements()) {
            std::pair<std::string, int> pair = {i, items.read(i)};
            if (pair.second > 0) {
                res.insert(pair);
            }
        }
        return res;
    }

    std::set<std::string> allElements() {
        return items.elements();
    }

    std::unordered_map<std::string, int> allElementsRead() {
        std::unordered_map<std::string, int> res;
        for (const auto& i : items.elements()) {
            std::pair<std::string, int> pair = {i, items.read(i)};
            res.insert(pair);
        }
        return res;
    }

    std::vector<Action> getHistory() { return history; }

    ShoppingListResponse createItem(const std::string& name, int quantity = 1) {
        ShoppingListResponse res;
        GSet<std::string> item_delta;
        GSet<std::string> acquired_delta;
        if (quantity < 1) {
            // invalid quantity
            res.first = false;
            item_delta.join(items);
            acquired_delta.join(items_acquired);

        } else if (items.contains(name)) {
            if (items.read(name) <= 0) {
                // Creates with target quantity if it exists but it has negative values or 0
                item_delta.join(items.inc(name, -items.read(name)));
                item_delta.join(items.inc(name, quantity));
                acquired_delta.join(items_acquired);
                addAction(ActionType::CREATE, name, quantity);
                res.first = true;
            } else {
                // Can't create item that exists
                res.first = false;
                item_delta.join(items);
                acquired_delta.join(items_acquired);
            }
        } else {
            // Creates if does not exist
            item_delta.join(items.add(name));
            item_delta.join(items.inc(name, quantity));
            acquired_delta.join(items_acquired.add(name));
            addAction(ActionType::CREATE, name, quantity);
            res.first = true;
        }
        res.second.first = item_delta;
        res.second.second = acquired_delta;
        return res;
    }

    ShoppingListResponse removeItem(const std::string& name) {
        ShoppingListResponse res;
        GSet<std::string> item_delta;
        GSet<std::string> acquired_delta;
        if (!contains(name)) {
            // does not have the item
            res.first = false;
            item_delta.join(items);
            acquired_delta.join(items_acquired);
        } else {
            // can remove
            item_delta.join(items.dec(name, items.read(name)));
            acquired_delta.join(items_acquired);
            res.first = true;
            addAction(ActionType::REMOVE, name);
        }
        res.second.first = item_delta;
        res.second.second = acquired_delta;
        return res;
    }

    ShoppingListResponse increaseItem(const std::string& name, int quantity = 1) {
        ShoppingListResponse res;
        GSet<std::string> item_delta;
        GSet<std::string> acquired_delta;
        if (!contains(name)) {
            res.first = false;
            item_delta.join(items);
            acquired_delta.join(items_acquired);

        } else {
            res.first = true;
            item_delta.join(items.inc(name, quantity));
            acquired_delta.join(items_acquired);

            addAction(ActionType::INCREASE, name, quantity);
        }
        res.second.first = item_delta;
        res.second.second = acquired_delta;
        return res;
    }

    ShoppingListResponse buyItem(const std::string& name, int quantity = 1) {
        ShoppingListResponse res;
        GSet<std::string> item_delta;
        GSet<std::string> acquired_delta;
        if (!contains(name)) {
            res.first = false;
            item_delta.join(items);
            acquired_delta.join(items_acquired);

        } else {
            res.first = true;
            item_delta.join(items.dec(name, quantity));
            acquired_delta.join(items_acquired.inc(name, quantity));
            addAction(ActionType::BUY, name, quantity);
        }
        res.second.first = item_delta;
        res.second.second = acquired_delta;
        return res;
    }

    ShoppingListResponse updateList(const ShoppingList& o) {
        ShoppingListResponse res;
        GSet<std::string> item_delta;
        GSet<std::string> acquired_delta;
        if (url == o.url && o.title == title) {
            item_delta = items.join(o.items);
            acquired_delta = items_acquired.join(o.items_acquired);
            res.first = true;

            // history needs working
            int i = 0, j = 0;
            while (i < history.size() && j < o.history.size()) {
                if (history[i] == o.history[j]) {
                    i++;
                    j++;
                }
            }
            int size = history.size();
            history.resize(history.size() + (o.history.size() - j));
            for (int k = 0; k < o.history.size() - j; k++) {
                history[size + k] = o.history[j + k];
            }
        } else {
            res.first = false;
            item_delta.join(items);
            acquired_delta.join(items_acquired);
        }
        res.second.first = item_delta;
        res.second.second = acquired_delta;
        return res;
    }
};

#endif