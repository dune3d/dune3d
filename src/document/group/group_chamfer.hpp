#pragma once
#include "group_local_operation.hpp"

namespace dune3d {

class GroupChamfer : public GroupLocalOperation {
public:
    explicit GroupChamfer(const UUID &uu);
    explicit GroupChamfer(const UUID &uu, const json &j);

    static constexpr Type s_type = Type::CHAMFER;
    Type get_type() const override
    {
        return s_type;
    }
    std::optional<double> m_radius2;

    json serialize() const override;

    void update_solid_model(const Document &doc) override;
    std::unique_ptr<Group> clone() const override;
};
} // namespace dune3d
