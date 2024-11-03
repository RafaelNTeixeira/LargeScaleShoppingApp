// #ifndef CAUSAL_CONTEXT_H
// #define CAUSAL_CONTEXT_H

// #include <algorithm>
// #include <string>
// #include <unordered_map>

// template <typename K = std::string>
// using UTag = std::pair<K, int>;

// template <typename K = std::string>
// class CausalContext {
//    public:
//     std::unordered_map<K, int> history;

//     CausalContext<K>& operator=(const CausalContext<K>& o) {
//         if (&o == this) return *this;
//         history = o.history;
//         return *this;
//     }

//     friend ostream& operator<<(ostream& output, const CausalContext<K>& o) {
//         output << "CausalContext:";
//         output << " History ( ";
//         for (const auto& p : o.history)
//             output << p.first << ":" << p.second << " ";
//         output << ")";
//         return output;
//     }

//     bool in(const UTag<K>& d) const {
//         const auto it = history.find(d.first);
//         return it != history.end() && d.second <= it->second;
//     }

//     UTag makeUTag(const K& id) {
//         auto response = history.insert({id, 1});
//         if (!response.second)
//             response.first->second = response.first->second + 1;
//         return UTag({response.first.first, response.first.second});
//     }

//     void join(const CausalContext<K>& o) {
//         if (this == &o) return;

//         for (auto& it : o.history) {
//             auto this_it = history.find(it.first);
//             if (this_it != history.end()) {
//                 history[it.first] = std::max(this_it.second, it.second);
//             } else {
//                 history[it.first] = it.second;
//             }
//         }
//     }
// };

// #endif
