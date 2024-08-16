#pragma once
#include <glm/glm.hpp>
#include <optional>

namespace dune3d {
template <typename T> class BBoxAccumulator {
    using TBB = std::pair<T, T>;

public:
    void accumulate(const T &c)
    {
        if (m_bbox) {
            m_bbox->first = glm::min(m_bbox->first, c);
            m_bbox->second = glm::max(m_bbox->second, c);
        }
        else {
            m_bbox.emplace(c, c);
        }
    }

    void accumulate(const TBB &bb)
    {
        accumulate(bb.first);
        accumulate(bb.second);
    }

    const auto &get() const
    {
        return m_bbox;
    }

    const TBB get_or_0() const
    {
        if (m_bbox)
            return m_bbox.value();
        else
            return {};
    }

    const TBB get_or(const TBB &other) const
    {
        if (m_bbox)
            return m_bbox.value();
        else
            return other;
    }

private:
    std::optional<TBB> m_bbox;
};
} // namespace dune3d
