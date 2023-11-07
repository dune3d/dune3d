#pragma once
#include "group.hpp"
#include "igroup_generate.hpp"
#include "igroup_solid_model.hpp"
#include <glm/glm.hpp>

namespace dune3d {

class Document;
class SolidModel;

class GroupSweep : public Group, public IGroupGenerate, public IGroupSolidModel {
public:
    explicit GroupSweep(const UUID &uu);
    explicit GroupSweep(const UUID &uu, const json &j);

    UUID m_wrkpl;
    UUID m_source_group;

    std::shared_ptr<const SolidModel> m_solid_model;

    enum class Operation { UNION, DIFFERENCE };
    Operation m_operation = Operation::DIFFERENCE;

    const SolidModel *get_solid_model() const override;

    std::list<GroupStatusMessage> m_sweep_messages;
    std::list<GroupStatusMessage> get_messages() const override;

    json serialize() const override;

    std::set<UUID> get_referenced_entities(const Document &doc) const override;
    std::set<UUID> get_referenced_groups(const Document &doc) const override;

    std::set<UUID> get_required_entities(const Document &doc) const override;
    std::set<UUID> get_required_groups(const Document &doc) const override;
};

} // namespace dune3d
