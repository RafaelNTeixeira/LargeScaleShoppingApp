#ifndef COUNTER_H
#define COUNTER_H

#include <algorithm>
#include <string>
#include <unordered_map>

template <typename V = int, typename K = std::string>
class BaseCounter {
   private:
    std::unordered_map<K, V> m;
    K id;

   public:
    BaseCounter() {}
    BaseCounter(K id) : id(id) {}
    BaseCounter(const BaseCounter<V, K>& o) : m(o.m), id(o.id) {}

    BaseCounter inc(V value = {1}) {
        BaseCounter<V, K> res;
        m[id] += value;
        res.m[id] = m[id];
        return res;
    }

    bool operator==(const BaseCounter<V, K>& o) const { return m == o.m; }

    V local() const {
        V res = 0;
        if (m.count(id) != 0) {
            res += m.at(id);
        }
        return res;
    }

    V read() const {
        V res = 0;
        for (const auto& kv : m)
            res += kv.second;
        return res;
    }

    void join(const BaseCounter<V, K>& o) {
        for (const auto& okv : o.m)
            m[okv.first] = std::max(okv.second, m[okv.first]);
    }

    friend std::ostream& operator<<(std::ostream& output, const BaseCounter<V, K>& o) {
        output << "BaseCounter: ( ";
        for (const auto& kv : o.m)
            output << kv.first << "->" << kv.second << " ";
        output << ")";
        return output;
    }
};

template <typename V = int, typename K = std::string>
class Counter {
   private:
    BaseCounter<V, K> i, d;  // increases and decreases

   public:
    Counter() {}
    Counter(K id) : i(id), d(id) {}
    Counter(const Counter<V, K>& o) : i(o.i), d(o.d) {}

    Counter inc(V value = {1}) {
        Counter<V, K> delta;
        delta.i = i.inc(value);
        return delta;
    }

    Counter dec(V value = {1}) {
        Counter<V, K> delta;
        delta.d = d.inc(value);
        return delta;
    }

    V local() const {
        return i.local() - d.local();
    }

    V read() const {
        return i.read() - d.read();
    }

    void join(const Counter& o) {
        i.join(o.i);
        d.join(o.d);
    }

    friend std::ostream& operator<<(std::ostream& output, const Counter<V, K>& o) {
        output << "Counter:I:" << o.i << " Counter:D:" << o.d;
        return output;
    }
};

#endif
