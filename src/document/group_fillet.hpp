#pragma once
#include "group_local_operation.hpp"

namespace dune3d {

class GroupFillet : public GroupLocalOperation {
public:
    using GroupLocalOperation::GroupLocalOperation;

    static constexpr Type s_type = Type::FILLET;
    Type get_type() const override
    {
        return s_type;
    }

    void update_solid_model(const Document &doc) override;
    std::unique_ptr<Group> clone() const override;
};
} // namespace dune3d
