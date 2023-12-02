#pragma once
#include "group.hpp"
#include "igroup_generate.hpp"
#include "igroup_solid_model.hpp"
#include <glm/glm.hpp>

namespace dune3d {

class Document;
class SolidModel;

class GroupLinearArray : public Group, public IGroupGenerate, public IGroupSolidModel {
public:
    explicit GroupLinearArray(const UUID &uu);
    explicit GroupLinearArray(const UUID &uu, const json &j);

    static constexpr Type s_type = Type::LINEAR_ARRAY;
    Type get_type() const override
    {
        return s_type;
    }

    UUID m_source_group;

    glm::dvec3 m_dvec = {1, 1, 0};
    unsigned int m_count = 3;

    enum class Offset {
        ZERO,
        ONE,
        PARAM,
    };
    Offset m_offset = Offset::ZERO;

    glm::dvec3 m_offset_vec = {0, 0, 0};

    Operation m_operation = Operation::DIFFERENCE;
    Operation get_operation() const override
    {
        return m_operation;
    }

    glm::dvec3 get_shift3(const Document &doc, unsigned int instance) const;
    glm::dvec2 get_shift2(unsigned int instance) const;

    std::shared_ptr<const SolidModel> m_solid_model;

    const SolidModel *get_solid_model() const override;
    void update_solid_model(const Document &doc) override;

    UUID get_entity_uuid(const UUID &uu, unsigned int instance) const;

    std::list<GroupStatusMessage> m_array_messages;
    std::list<GroupStatusMessage> get_messages() const override;

    void generate(Document &doc) const override;

    json serialize() const override;
    std::unique_ptr<Group> clone() const override;

    std::set<UUID> get_referenced_entities(const Document &doc) const override;
    std::set<UUID> get_referenced_groups(const Document &doc) const override;

    std::set<UUID> get_required_entities(const Document &doc) const override;
    std::set<UUID> get_required_groups(const Document &doc) const override;

private:
    glm::dvec3 get_shift(unsigned int instance) const;
};

} // namespace dune3d
