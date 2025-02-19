#pragma once
#include "group_sweep.hpp"
#include "document/entity/entity_and_point.hpp"
#include <glm/glm.hpp>

namespace dune3d {

class Document;
class SolidModel;
enum class EntityType;

class GroupPipe : public GroupSweep {
public:
    explicit GroupPipe(const UUID &uu);
    explicit GroupPipe(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::PIPE;
    Type get_type() const override
    {
        return s_type;
    }

    std::set<UUID> m_spine_entities;
    EntityAndPoint m_start_point;

    void update_solid_model(const Document &doc) override;

    void generate(Document &doc) override;

    json serialize() const override;
    std::unique_ptr<Group> clone() const override;

    static bool entity_type_is_supported(EntityType type);

    std::set<UUID> get_referenced_entities(const Document &doc) const override;
    std::set<UUID> get_required_entities(const Document &doc) const override;
};

} // namespace dune3d
