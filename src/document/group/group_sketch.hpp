#pragma once
#include "group.hpp"
#include "igroup_solid_model.hpp"

namespace dune3d {
class GroupSketch : public Group, public IGroupSolidModel {
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


    std::shared_ptr<const SolidModel> m_solid_model;

    Operation m_operation = Operation::UNION;
    Operation get_operation() const override
    {
        return m_operation;
    }
    void set_operation(Operation op) override
    {
        m_operation = op;
    }

    const SolidModel *get_solid_model() const override;
    void update_solid_model(const Document &doc) override;
};

} // namespace dune3d
