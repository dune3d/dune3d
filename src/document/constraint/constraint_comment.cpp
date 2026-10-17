#include "constraint_comment.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "constraintt_impl.hpp"
#include "util/glm_util.hpp"
#include "util/get_entity_origin.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/entity/entity_visitor.hpp"

namespace dune3d {
ConstraintComment::ConstraintComment(const UUID &uu) : Base(uu)
{
}

ConstraintComment::ConstraintComment(const UUID &uu, const json &j)
    : Base(uu, j), m_point(j.at("point").get<EntityAndPoint>()), m_offset(j.at("offset").get<glm::dvec3>()),
      m_wrkpl(j.at("wrkpl").get<UUID>()), m_comment(j.at("comment").get<std::string>())
{
}

json ConstraintComment::serialize() const
{
    json j = Constraint::serialize();
    j["point"] = m_point;
    j["wrkpl"] = m_wrkpl;
    j["offset"] = m_offset;
    j["comment"] = m_comment;
    return j;
}

const UUID &ConstraintComment::get_workplane(const Document &doc) const
{
    return m_wrkpl;
}

glm::dvec3 ConstraintComment::get_origin(const Document &doc) const
{
    return get_entity_origin(doc, m_point);
}

glm::dvec3 ConstraintComment::get_offset() const
{
    return m_offset;
}

void ConstraintComment::set_offset(const glm::dvec3 &offset)
{
    m_offset = offset;
}

bool ConstraintComment::offset_is_in_workplane() const
{
    return false;
}

} // namespace dune3d
