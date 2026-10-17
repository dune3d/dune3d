#pragma once
#include <glm/glm.hpp>

namespace dune3d {

class EntityAndPoint;
class Document;

glm::dvec3 get_entity_origin(const Document &doc, const EntityAndPoint &enp);

} // namespace dune3d
