#pragma once
#include "group.hpp"

namespace dune3d {
class GroupSketch : public Group {
public:
    explicit GroupSketch(const UUID &uu);
    explicit GroupSketch(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::SKETCH;
    Type get_type() const override
    {
        return s_type;
    }
    json serialize() const override;
    std::unique_ptr<Group> clone() const override;
};

} // namespace dune3d
