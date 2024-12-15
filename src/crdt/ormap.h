#ifndef AWORSET_H
#define AWORSET_H

#include <algorithm>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
#include <string>

#include "join.h"

typedef std::pair<std::string, long> Dot;
typedef std::map<std::string, long> VectorClock;

class DotContext {
   public:
    VectorClock vc;
    std::set<Dot> dc;

    bool operator==(const DotContext& o) const {
        return vc == o.vc && dc == o.dc;
    }

    DotContext& operator=(const DotContext& o) {
        if (&o == this) return *this;
        vc = o.vc;
        dc = o.dc;
        return *this;
    }

    bool contains(const Dot& d) const {
        const auto& it = vc.find(d.first);
        if (it == vc.end()) {
            return dc.count(d) > 0;
        }
        return it->second >= d.second || dc.count(d) > 0;
    }

    Dot nextDot(const std::string& id) {
        auto r = vc.insert({id, 1});
        if (!r.second) {
            r.first->second++;
        }
        return {id, r.first->second};
    }

    void join(const DotContext& o) {
        if (this == &o) {
            return;
        }

        auto mit = vc.begin();
        auto oit = o.vc.begin();

        while (mit != vc.end() && oit != o.vc.end()) {
            if (mit->first < oit->first) {
                mit++;
            } else if (mit->first > oit->first) {
                vc.insert(*oit);
                oit++;
            } else {
                if (mit->second < oit->second) {
                    mit->second = oit->second;
                }
                mit++;
                oit++;
            }
        }

        while (oit != o.vc.end()) {
            vc.insert(*oit);
            oit++;
        }

        for (const auto& d : o.dc) {
            dc.insert(d);
        }

        compact();
    }

    void compact() {
        bool flag = true;
        while (flag) {
            flag = false;
            for (auto it = dc.begin(); it != dc.end();) {
                auto ccit = vc.find(it->first);
                if (ccit == vc.end()) {
                    if (it->second == 0) {
                        vc.insert(*it);
                        dc.erase(it++);
                        flag = true;
                    } else {
                        it++;
                        continue;
                    }
                } else if (vc.at(it->first) + 1 == it->second) {
                    vc[it->first]++;
                    dc.erase(it++);
                    flag = true;
                } else if (vc.at(it->first) >= it->second) {
                    dc.erase(it++);
                    flag = true;
                } else {
                    it++;
                }
            }
        }
    }

    nlohmann::json toJson() const {
        nlohmann::json j;
        j["vc"] = vc;
        j["dc"] = dc;
        return j;
    }

    void fromJson(const nlohmann::json& j) {
        vc = j["vc"].get<VectorClock>();
        for (const auto& d : j["dc"]) {
            dc.insert({d[0], d[1]});
        }
    }
};

template <typename T>
class DotKernel {
   public:
    DotContext base;
    DotContext& context;
    std::map<Dot, T> values;

    DotKernel() : context(base) {}
    DotKernel(DotContext& context) : context(context) {}
    DotKernel(const DotKernel<T>& o) : context(o.context), values(o.values) {}

    DotKernel<T>& operator=(const DotKernel<T>& o) {
        if (this == &o) {
            return *this;
        }
        context = o.context;
        values = o.values;
        return *this;
    }

    bool operator==(const DotKernel<T>& o) const {
        return context == o.context && values == o.values;
    }

    std::set<T> allValues() const {
        std::set<T> res;
        for (const auto& v : values) {
            res.insert(v.second);
        }
        return res;
    }

    void join(const DotKernel<T>& o) {
        auto it = values.begin();
        auto oit = o.values.begin();

        while (it != values.end() && oit != o.values.end()) {
            if (it->first < oit->first) {
                if (o.context.contains(it->first)) {
                    values.erase(it++);
                } else {
                    it++;
                }
            } else if (it->first > oit->first) {
                if (!context.contains(oit->first)) {
                    values.insert(*oit);
                }
                oit++;
            } else {
                if (it->second != oit->second) {
                    join::join(it->second, oit->second);
                }
                it++;
                oit++;
            }
        }

        while (it != values.end()) {
            if (o.context.contains(it->first)) {
                values.erase(it++);
            } else {
                it++;
            }
        }

        while (oit != o.values.end()) {
            if (!context.contains(oit->first)) {
                values.insert(*oit);
            }
            oit++;
        }

        context.join(o.context);
    }

    void add(const std::string& replica_id, const T& v) {
        Dot d = context.nextDot(replica_id);
        this->values.insert({d, v});
    }

    void remove(const T& v) {
        for (auto it = values.begin(); it != values.end(); it++) {
            if (it->second == v) {
                values.erase(it);
            }
        }
    }

    void remove(const Dot& d) {
        values.erase(d);
    }

    void remove() {
        values.clear();
    }

    nlohmann::json toJson() const {
        nlohmann::json j;
        j["values"] = values;
        j["context"] = context.toJson();
        return j;
    }
};

template <typename T>
class CausalCounter {
   public:
    DotKernel<T> core;
    CausalCounter() = default;
    CausalCounter(DotContext& context) : core(context) {}
    CausalCounter(const CausalCounter<T>& o) : core(o.core) {}

    bool operator==(const CausalCounter<T>& o) const {
        return core == o.core;
    }

    T local(const std::string& replica_id) const {
        T res = 0;
        for (const auto& v : core.values) {
            if (v.first.first == replica_id) {
                res += v.second;
            }
        }
        return res;
    }

    T read() const {
        T res = 0;
        for (const auto& v : core.values) {
            res += v.second;
        }
        return res;
    }

    void inc(const std::string& replica_id, const T& v = 1) {
        std::set<Dot> dots;
        T value = {};
        for (const auto& it : core.values) {
            if (it.first.first == replica_id) {
                value = std::max(value, it.second);
                dots.insert(it.first);
            }
        }
        for (const auto& d : dots) {
            core.remove(d);
        }
        core.add(replica_id, value + v);
    }

    void dec(const std::string& replica_id, const T& v = 1) {
        // Does not work when read value is negative
        std::set<Dot> dots;
        T value = {};
        for (const auto& it : core.values) {
            if (it.first.first == replica_id) {
                value = std::max(value, it.second);
                dots.insert(it.first);
            }
        }
        for (const auto& d : dots) {
            core.remove(d);
        }
        core.add(replica_id, value - v);
    }

    void join(const CausalCounter<T>& o) {
        core.join(o.core);
    }

    void reset() {
        core.remove();
    }

    nlohmann::json toJson() const {
        return core.toJson();
    }
};

// template <typename T>
// class AWORSet {
//    public:
//     DotKernel<T> core;

//     AWORSet() = default;
//     AWORSet(const AWORSet<T>& o) : core(o.core) {}

//     AWORSet<T>& operator=(const AWORSet<T>& o) {
//         if (this == &o) {
//             return *this;
//         }
//         core = o.core;
//         return *this;
//     }

//     bool operator==(const AWORSet<T>& o) const {
//         return core == o.core;
//     }

//     std::set<T> allValues() const {
//         std::set<T> res;
//         for (const auto& v : core.values) {
//             res.insert(v.second);
//         }
//         return res;
//     }

//     AWORSet<T> add(const std::string& replica_id, const T& v) {
//         AWORSet<T> res;
//         res.core = core.remove(v);
//         res.core.join(core.add(replica_id, v));
//         return res;
//     }

//     AWORSet<T> remove(const T& v) {
//         AWORSet<T> res;
//         res.core = core.remove(v);
//         return res;
//     }

//     void join(const AWORSet<T>& o) {
//         core.join(o.core);
//     }
// };

template <typename K, typename V>
class ORMap {
   public:
    std::map<K, V> map;
    DotContext base;
    DotContext& context;

    ORMap() : context(base) {}
    ORMap(DotContext& context) : context(context) {}
    ORMap(const ORMap<K, V>& o) : map(o.map), context(o.context) {}

    ORMap<K, V>& operator=(const ORMap<K, V>& o) {
        if (this == &o) {
            return *this;
        }
        map = o.map;
        context = o.context;
        return *this;
    }

    bool operator==(const ORMap<K, V>& o) const {
        return map == o.map && context == o.context;
    }

    V& operator[](const K& k) {
        auto it = map.find(k);
        if (it == map.end()) {
            it = map.insert(it, {k, V(context)});
        }
        return it->second;
    }

    void remove(const K& k) {
        map[k].reset();
        map.erase(k);
    }

    void reset() {
        for (auto& kv : map) {
            kv.second.reset();
        }
        map.clear();
    }

    void join(const ORMap<K, V>& o) {
        const DotContext initial_context = context;

        auto it = map.begin();
        auto oit = o.map.begin();

        while (it != map.end() && oit != o.map.end()) {
            if (it->first < oit->first) {
                V v(o.context);
                it->second.join(v);
                it++;
            } else if (it->first > oit->first) {
                (*this)[oit->first].join(oit->second);
                oit++;
            } else {
                (*this)[it->first].join(oit->second);
                it++;
                oit++;
            }
            context = initial_context;
        }

        while (it != map.end()) {
            V v(o.context);
            it->second.join(v);
            context = initial_context;
            it++;
        }

        while (oit != o.map.end()) {
            (*this)[oit->first].join(oit->second);
            oit++;
            context = initial_context;
        }

        context.join(o.context);
    }

    nlohmann::json toJson() const {
        nlohmann::json j;
        nlohmann::json map;
        for (const auto& kv : this->map) {
            map[kv.first] = kv.second.toJson();
        }
        j["map"] = map;
        j["context"] = context.toJson();
        return j;
    }
};

#endif
