#ifndef JOIN_H
#define JOIN_H

template <typename T>
T join(const T& l, const T& r) {
    T res = l;
    res.join(r);
    return res;
}

#endif
