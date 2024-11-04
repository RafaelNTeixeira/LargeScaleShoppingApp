#ifndef GSET_H
#define GSET_H

#include <algorithm>
#include <set>
#include <string>
#include <unordered_map>

#include "causal_context.h"
#include "counter.h"
#include "join.h"

// Adpated to our purposes
template <typename K = std::string>
class GSet {
   private:
    K id;
    std::unordered_map<K, Counter<int, K>> set;

   public:
    GSet() {}
    GSet(K id) : id(id) {}
    GSet(const GSet<K>& o) : id(o.id), set(o.set) {}

    bool contains(const std::string& element) const {
        auto item = set.find(element);
        return item != set.end();
    }

    std::set<K> elements() const {
        std::set<K> res;
        for (const auto& e : set) {
            res.insert(e.first);
        }
        return res;
    }

    GSet<K> add(const K& element) {
        GSet<K> res;
        if (!contains(element)) {
            // Adds if not exists
            set[element] = Counter<int, K>(id);
        }
        res.set = set;
        return res;
    }

    std::pair<Counter<int, K>*, bool> get(const K& element) {
        if (contains(element)) {
            return {&set.at(element), true};
        }
        return {nullptr, false};
    }

    GSet<K> inc(const K& element, int value = 1) {
        GSet<K> res;
        if (contains(element)) {
            set[element].inc(value);
        }
        res.set = set;
        return res;
    }

    GSet<K> dec(const K& element, int value = 1) {
        GSet<K> res;
        if (contains(element)) {
            set[element].dec(value);
        }
        res.set = set;
        return res;
    }

    int read(const K& element) const {
        if (contains(element)) {
            return set.at(element).read();
        }
        return 0;
    }

    GSet<K> join(const GSet<K>& o) {
        GSet<K> res;
        for (auto it = o.set.begin(); it != o.set.end(); it++) {
            if (contains(it->first)) {
                set[it->first] = ::join(set[it->first], it->second);
            } else {
                set[it->first] = it->second;
            }
        }
        res.set = set;
        return res;
    }

    friend std::ostream&
    operator<<(std::ostream& output, const GSet<K>& o) {
        output << "Gset: ( ";
        for (const auto& p : o.set) {
            output << p.first << " -> " << p.second << ", ";
        }
        output << ")";
        return output;
    }
};

#endif
