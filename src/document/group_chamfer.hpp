#pragma once
#include "group_local_operation.hpp"

namespace dune3d {

class GroupChamfer : public GroupLocalOperation {
public:
    using GroupLocalOperation::GroupLocalOperation;

    static constexpr Type s_type = Type::CHAMFER;
    Type get_type() const override
    {
        return s_type;
    }

    void update_solid_model(const Document &doc) override;
    std::unique_ptr<Group> clone() const override;
};
} // namespace dune3d
