// Not sure how to do the join in some situations

// #ifndef AWORSET_H
// #define AWORSET_H

// #include <algorithm>
// #include <set>
// #include <string>
// #include <unordered_map>

// #include "causal_context.h"
// #include "counter.h"

// /*
// Quero uma estrutura que me permita através de uma Key, mapear para um value
// Além disso, tenho de ter tag unicas que me indicam quem adicionou o que.
// */

// // Adpated for our purpose
// template <typename K = std::string, typename V = std::string>
// class Aworset {
//    private:
//     K id;
//     CausalContext<K> cc;
//     std::unordered_map<std::string, std::pair<Counter, UTag<K>>> set;

//    public:
//     Aworset() {}
//     Aworset(K id) : id(id) {}

//     bool contains(const std::string& element) {
//         auto item = set.find(element);
//         return item != set.end();
//     }

//     std::set<std::string> elements() const {
//         std::set<std::string> res;
//         for (const auto& e : set) {
//             res.insert(e.first);
//         }
//         return res;
//     }

//     Aworset<K> add(const std::string& element) {
//         Aworset<K> res;
//         UTag<K> tag = cc.makeUTag();
//         set[element] = {Counter(id), tag};  // overwrites what is in the set
//         res.cc = cc;
//         res.set = set;
//         return res;
//     }

//     Aworset<K> remove(const std::string& element) {
//         Aworset<K> res;
//         set.erase(element);
//         res.set = set;
//         res.cc = cc;
//         return res;
//     }

//     Aworset<K> join(const Aworset<K>& o) {
//         Aworset<K> res;

//         unordered_map<K, std::pair<Counter, UTag>> mi;   // M'
//         unordered_map<K, std::pair<Counter, UTag>> mii;  // M''
//         unordered_map<K, std::pair<Counter, UTag>> m;    // M

//         for (auto it = set.begin(); it != set.end(); it++) {
//             if (o.contains(it.first)) {
//                 // both contain item
//                 auto oinfo = o.set[it.first];
//                 if (info.second == oinfo.second) {
//                     // both contain item and the same tag
//                     m[it.first] = {join(it.second.first, oinfo.first), info.second};
//                 } else {
//                     // both contain item but tags are different
//                     if (!cc.in(oinfo.second)) {
//                         // self does not contain o tag
//                         mii[it.first] = oinfo;
//                     }
//                     if (!o.cc.in(it.second.second)) {
//                         // o does not contain self tag
//                         mi[it.first] = it.second;
//                     }
//                 }
//             } else {
//                 // o does not contain item
//                 mi[it.first] = it.second;
//             }
//         }

//         // Not in self
//         for (auto it = o.set.begin(); it != set.end(); it++) {
//             if (!contains(it.first)) {
//                 mii[it.first] = it.second;
//             }
//         }

//         // joining
//         for
//     }

//     friend std::ostream&
//     operator<<(std::ostream& output, const Aworset<V, K>& o) {
//         output << "Aworset: ( ";
//         output << ")";
//         return output;
//     }
// };

// #endif
