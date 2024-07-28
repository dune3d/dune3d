#include "constraint_point_on_bezier.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "constraint_visitor.hpp"
#include "document/document.hpp"
#include "document/entity/entity_bezier2d.hpp"
#include "document/entity/entity_workplane.hpp"

namespace dune3d {
std::unique_ptr<Constraint> ConstraintPointOnBezier::clone() const
{
    return std::make_unique<ConstraintPointOnBezier>(*this);
}

void ConstraintPointOnBezier::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
}

void ConstraintPointOnBezier::modify_to_satisfy(const Document &doc)
{
    const auto &bezier = doc.get_entity<EntityBezier2D>(m_line);
    const auto &wrkpl = doc.get_entity<EntityWorkplane>(m_wrkpl);
    const auto point = wrkpl.project(doc.get_point(m_point));
    double best_t = 0;
    double best_distance = INFINITY;
    unsigned int steps = 64;
    for (unsigned int i = 1; i <= steps; i++) {
        const auto t = (double)i / steps;
        const auto p = bezier.get_interpolated(t);
        const auto dist = glm::length(p - point);
        if (dist < best_distance) {
            best_distance = dist;
            best_t = t;
        }
    }
    m_val = best_t;
}

} // namespace dune3d
