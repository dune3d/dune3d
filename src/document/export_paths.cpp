#include "export_paths.hpp"
#include "document.hpp"
#include "group/group.hpp"
#include "entity/entity_circle2d.hpp"
#include "entity/entity_line2d.hpp"
#include "entity/entity_arc2d.hpp"
#include "entity/entity_bezier2d.hpp"
#include "entity/entity_workplane.hpp"
#include "solid_model_util.hpp"
#include "util/fs_util.hpp"
#include <cairomm/cairomm.h>

namespace dune3d {
void export_paths(const std::filesystem::path &filename, const Document &doc, const UUID &group_uu,
                  std::function<bool(const Group &)> group_filter)
{
    auto &cur_group = doc.get_group(group_uu);
    if (!cur_group.m_active_wrkpl)
        throw std::runtime_error("needs workplane");
    auto &ref_wrkpl = doc.get_entity<EntityWorkplane>(cur_group.m_active_wrkpl);


    auto groups_sorted = doc.get_groups_sorted();
    auto surf = Cairo::RecordingSurface::create();
    auto ctx = Cairo::Context::create(surf);
    ctx->set_source_rgb(0, 0, 0);
    ctx->set_line_width(0.1);

    for (auto group : groups_sorted) {
        if (!group->m_active_wrkpl)
            continue;

        if (group->get_index() > cur_group.get_index())
            break;

        if (group_filter && !group_filter(*group))
            continue;

        const auto paths = solid_model_util::Paths::from_document(doc, group->m_active_wrkpl, group->m_uuid);

        if (paths.paths.size() == 0)
            continue;

        auto &wrkpl = doc.get_entity<EntityWorkplane>(group->m_active_wrkpl);
        auto delta_normal = glm::inverse(ref_wrkpl.m_normal) * wrkpl.m_normal;
        auto mat = glm::toMat3(delta_normal);

        const auto p0 = ref_wrkpl.project(wrkpl.m_origin);
        Cairo::Matrix cmat(mat[0][0], -mat[0][1], mat[1][0], -mat[1][1], p0.x, -p0.y);
        try {
            Cairo::Matrix imat = cmat;
            imat.invert();
        }
        catch (...) {
            continue;
        }


        for (const auto &path : paths.paths) {
            bool first = true;
            ctx->save();
            ctx->set_matrix(cmat);
            if (auto rad = dynamic_cast<const IEntityRadius *>(&path.front().second.entity); rad && path.size() == 1) {
                const auto center = rad->get_center();
                ctx->arc(center.x, center.y, rad->get_radius(), 0, 2 * M_PI);
            }
            else {
                for (auto &[node, edge] : path) {
                    const auto p_cur = node.get_pt_for_edge(edge);
                    const auto p_next = p_cur == 1 ? 2 : 1;
                    if (auto line = dynamic_cast<const EntityLine2D *>(&edge.entity)) {
                        if (first) {
                            const auto p = line->get_point_in_workplane(p_cur);
                            ctx->move_to(p.x, p.y);
                        }

                        const auto p = line->get_point_in_workplane(p_next);
                        ctx->line_to(p.x, p.y);
                    }
                    else if (auto arc = dynamic_cast<const EntityArc2D *>(&edge.entity)) {
                        auto v1 = arc->get_point_in_workplane(p_cur) - arc->m_center;
                        auto v2 = arc->get_point_in_workplane(p_next) - arc->m_center;
                        if (p_cur == 1)
                            ctx->arc(arc->m_center.x, arc->m_center.y, arc->get_radius(), atan2(v1.y, v1.x),
                                     atan2(v2.y, v2.x));
                        else
                            ctx->arc_negative(arc->m_center.x, arc->m_center.y, arc->get_radius(), atan2(v1.y, v1.x),
                                              atan2(v2.y, v2.x));
                    }
                    else if (auto bez = dynamic_cast<const EntityBezier2D *>(&edge.entity)) {
                        if (first) {
                            const auto p = bez->get_point_in_workplane(p_cur);
                            ctx->move_to(p.x, p.y);
                        }

                        const auto c1 = bez->get_point_in_workplane(p_next == 1 ? 4 : 3);
                        const auto c2 = bez->get_point_in_workplane(p_next == 1 ? 3 : 4);
                        const auto p = bez->get_point_in_workplane(p_next);

                        ctx->curve_to(c1.x, c1.y, c2.x, c2.y, p.x, p.y);
                    }
                    first = false;
                }
            }
            ctx->close_path();
            ctx->restore();
            ctx->stroke();
        }
    }


    auto extents = surf->ink_extents();

    auto isurf = Cairo::SvgSurface::create(path_to_string(filename), extents.width, extents.height);
    cairo_svg_surface_set_document_unit(isurf->cobj(), CAIRO_SVG_UNIT_MM);
    {
        auto icr = Cairo::Context::create(isurf);
        icr->set_source(surf, -extents.x, -extents.y);
        icr->paint();
    }
}
} // namespace dune3d
