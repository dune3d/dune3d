#pragma once
#include "group_sweep.hpp"
#include "igroup_pre_solve.hpp"
#include "document/entity/entity_and_point.hpp"
#include <glm/glm.hpp>

namespace dune3d {

class Document;
class SolidModel;

class GroupCircularSweep : public GroupSweep {
public:
    explicit GroupCircularSweep(const UUID &uu);
    explicit GroupCircularSweep(const UUID &uu, const json &j);

    UUID m_normal;
    EntityAndPoint m_origin;

    std::optional<glm::dvec3> get_direction(const Document &doc) const;

    json serialize() const override;

    std::set<UUID> get_referenced_entities(const Document &doc) const override;

    std::set<UUID> get_required_entities(const Document &doc) const override;
};

} // namespace dune3d
