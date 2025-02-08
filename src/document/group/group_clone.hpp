#pragma once
#include "group.hpp"
#include "igroup_generate.hpp"
#include "igroup_source_group.hpp"
#include <glm/glm.hpp>

namespace dune3d {

class Document;
class SolidModel;
class Entity;

class GroupClone : public Group, public IGroupGenerate, public IGroupSourceGroup {
public:
    explicit GroupClone(const UUID &uu);
    explicit GroupClone(const UUID &uu, const json &j);

    static constexpr Type s_type = Type::CLONE;
    Type get_type() const override
    {
        return s_type;
    }

    UUID m_source_wrkpl;
    UUID m_source_group;

    std::set<UUID> get_source_groups() const override
    {
        return {m_source_group};
    }

    UUID get_entity_uuid(const UUID &uu) const;

    std::list<GroupStatusMessage> m_clone_messages;
    std::list<GroupStatusMessage> get_messages() const override;

    void generate(Document &doc) const override;

    json serialize() const override;
    std::unique_ptr<Group> clone() const override;

    std::set<UUID> get_referenced_entities(const Document &doc) const override;
    std::set<UUID> get_referenced_groups(const Document &doc) const override;

    std::set<UUID> get_required_entities(const Document &doc) const override;
    std::set<UUID> get_required_groups(const Document &doc) const override;
};

} // namespace dune3d
