#ifndef JOIN_H
#define JOIN_H

#include <algorithm>

namespace join {

// Template function for joining generic types.
template <typename T>
T join(const T& l, const T& r) {
    T res = l;
    res.join(r);
    return res;
}

// Overloaded function for joining integers.
int join(int l, int r) {
    int res;
    l = std::max(l, r);
    return l;
}

}  // namespace join

#endif
