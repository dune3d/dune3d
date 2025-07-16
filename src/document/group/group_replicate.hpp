#pragma once
#include "group.hpp"
#include "igroup_generate.hpp"
#include "igroup_solid_model.hpp"
#include "igroup_source_group.hpp"
#include <glm/glm.hpp>

namespace dune3d {

class Document;
class SolidModel;
class Entity;

class GroupReplicate : public Group, public IGroupGenerate, public IGroupSolidModel, public IGroupSourceGroup {
public:
    explicit GroupReplicate(const UUID &uu);
    explicit GroupReplicate(const UUID &uu, const json &j);

    UUID m_source_group;

    std::set<UUID> get_source_groups() const override
    {
        return {m_source_group};
    }

    bool m_use_acc = false;

    Operation m_operation = Operation::DIFFERENCE;
    Operation get_operation() const override
    {
        return m_operation;
    }
    void set_operation(Operation op) override
    {
        m_operation = op;
    }

    std::shared_ptr<const SolidModel> m_solid_model;

    const SolidModel *get_solid_model() const override;

    UUID get_entity_uuid(const UUID &uu, unsigned int instance) const;

    std::list<GroupStatusMessage> m_array_messages;
    std::list<GroupStatusMessage> get_messages() const override;

    void generate(Document &doc) override;

    json serialize() const override;

    std::set<UUID> get_referenced_entities(const Document &doc) const override;
    std::set<UUID> get_referenced_groups(const Document &doc) const override;

    std::set<UUID> get_required_entities(const Document &doc) const override;
    std::set<UUID> get_required_groups(const Document &doc) const override;

    virtual unsigned int get_count() const = 0;
    virtual bool get_mirror_arc(unsigned int instance) const
    {
        return false;
    }

protected:
    virtual glm::dvec2 transform(const glm::dvec2 &p, unsigned int instance) const = 0;
    virtual glm::dvec3 transform(const Document &doc, const glm::dvec3 &p, unsigned int instance) const = 0;
    virtual glm::dquat transform_normal(const Document &doc, const glm::dquat &q, unsigned int instance) const;
    virtual void post_add(Entity &new_entity, const Entity &entity, unsigned int instance) const;
};

} // namespace dune3d
