#ifndef COUNTER_H
#define COUNTER_H

#include <algorithm>
#include <string>
#include <unordered_map>

template <typename V = int, typename K = std::string>
class BaseCounter {
   private:
    std::unordered_map<K, V> m;

   public:
    BaseCounter() {}
    BaseCounter(const BaseCounter<V, K>& o) : m(o.m) {}

    BaseCounter inc(const K& replica_id, const V value = {1}) {
        BaseCounter<V, K> res;
        m[replica_id] += value;
        res.m[replica_id] = m[replica_id];
        return res;
    }

    bool operator==(const BaseCounter<V, K>& o) const { return m == o.m; }

    V local(const K& replica_id) const {
        V res = 0;
        if (m.count(replica_id) != 0) {
            res += m.at(replica_id);
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

    void reset() {
        m.clear();
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
    Counter(const Counter<V, K>& o) : i(o.i), d(o.d) {}

    bool operator==(const Counter<V, K>& o) const { return i == o.i && d == o.d; }

    Counter inc(const K& replica_id, V value = {1}) {
        Counter<V, K> delta;
        delta.i = i.inc(replica_id, value);
        return delta;
    }

    Counter dec(const K& replica_id, V value = {1}) {
        Counter<V, K> delta;
        delta.d = d.inc(replica_id, value);
        return delta;
    }

    V local(const K& replica_id) const {
        return i.local(replica_id) - d.local(replica_id);
    }

    V read() const {
        return i.read() - d.read();
    }

    void join(const Counter& o) {
        i.join(o.i);
        d.join(o.d);
    }

    void reset() {
        i.reset();
        d.reset();
    }

    friend std::ostream& operator<<(std::ostream& output, const Counter<V, K>& o) {
        output << "Counter:I:" << o.i << " Counter:D:" << o.d;
        return output;
    }
};

#endif
