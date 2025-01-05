#pragma once
#include "group_replicate.hpp"

namespace dune3d {

class Document;
class SolidModel;

class GroupArray : public GroupReplicate {
public:
    explicit GroupArray(const UUID &uu);
    explicit GroupArray(const UUID &uu, const json &j);

    unsigned int m_count = 3;

    enum class Offset {
        ZERO,
        ONE,
        PARAM,
    };
    Offset m_offset = Offset::ZERO;

    json serialize() const override;

protected:
    unsigned int get_count() const override
    {
        return m_count;
    }
};

} // namespace dune3d
