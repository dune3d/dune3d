#pragma once
#include "group.hpp"
#include "igroup_generate.hpp"
#include "igroup_solid_model.hpp"
#include "document/entity/entity.hpp"
#include <glm/glm.hpp>
#include <set>

namespace dune3d {

class Document;
class SolidModel;

class GroupLocalOperation : public Group, public IGroupSolidModel {
public:
    explicit GroupLocalOperation(const UUID &uu);
    explicit GroupLocalOperation(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::FILLET;

    // this is here for compatibility
    std::set<unsigned int> m_edges;
    std::set<UUID> m_entities;
    double m_radius = 0.1;

    Operation m_operation = Operation::DIFFERENCE;
    Operation get_operation() const override
    {
        return m_operation;
    }
    void set_operation(Operation op) override
    {
    }

    json serialize() const override;

    std::list<GroupStatusMessage> m_local_operation_messages;
    std::list<GroupStatusMessage> get_messages() const override;

    std::shared_ptr<const SolidModel> m_solid_model;

    const SolidModel *get_solid_model() const override;
    static bool entity_type_is_supported(EntityType type);
};
} // namespace dune3d
