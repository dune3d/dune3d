#pragma once
#include <algorithm>

namespace dune3d {
template <typename T> class MinMaxAccumulator {
public:
    MinMaxAccumulator() : mi(), ma()
    {
    }
    void accumulate(const T &v)
    {
        if (!isfinite(v))
            return;
        if (first) {
            mi = v;
            ma = v;
            first = false;
        }
        else {
            mi = std::min(mi, v);
            ma = std::max(ma, v);
        }
    }
    T get_max() const
    {
        return ma;
    }
    T get_min() const
    {
        return mi;
    }

private:
    T mi;
    T ma;
    bool first = true;
};
} // namespace dune3d
