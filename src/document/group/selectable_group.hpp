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

class SelectableGroup : public Group {
public:
    explicit SelectableGroup(const UUID &uu);
    explicit SelectableGroup(const UUID &uu, const json &j);

    std::set<UUID> m_entities;
};
} // namespace dune3d
