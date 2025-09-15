#include "export_dxf.hpp"
#include "document.hpp"
#include "group/group.hpp"
#include "entity/entity_line2d.hpp"
#include "entity/entity_arc2d.hpp"
#include "entity/entity_bezier2d.hpp"
#include "util/paths.hpp"
#include "util/fs_util.hpp"
#include "dxflib/dl_dxf.h"

namespace dune3d {

namespace {

class DxfWrapper {
public:
    DxfWrapper(const std::filesystem::path &path)
    {
        const auto filename_str = path_to_string(path);
        m_writer = m_dxf.out(filename_str.c_str());
        if (!m_writer)
            throw std::runtime_error("couldn't open file for writing");

        m_dxf.writeHeader(*m_writer);
        m_writer->sectionEnd();

        m_writer->sectionEntities();
    }

    template <typename Td, typename... Args>
    void write(void (DL_Dxf ::*f)(DL_WriterA &, const Td &, const DL_Attributes &), Args &&...args)
    {
        std::invoke(f, m_dxf, *m_writer, Td(std::forward<Args>(args)...), DL_Attributes{});
    }
    template <typename Td, typename... Args> void write(void (DL_Dxf ::*f)(DL_WriterA &, const Td &), Args &&...args)
    {
        std::invoke(f, m_dxf, *m_writer, Td(std::forward<Args>(args)...));
    }

    ~DxfWrapper()
    {
        m_writer->sectionEnd();
        m_dxf.writeObjects(*m_writer);
        m_dxf.writeObjectsEnd(*m_writer);
        m_writer->dxfEOF();
        m_writer->close();

        delete m_writer;
    }

private:
    DL_Dxf m_dxf;
    DL_WriterA *m_writer;
};
} // namespace

void export_dxf(const std::filesystem::path &filename, const Document &doc, const UUID &group_uu)
{
    auto &group = doc.get_group(group_uu);
    if (!group.m_active_wrkpl)
        throw std::runtime_error("needs workplane");

    const auto paths = paths::Paths::from_document(doc, group.m_active_wrkpl, group.m_uuid);
    if (paths.paths.size() == 0)
        return;

    DxfWrapper dxf(filename);

    for (const auto &path : paths.paths) {
        if (auto rad = dynamic_cast<const IEntityRadius *>(&path.front().second.entity); rad && path.size() == 1) {
            const auto center = path.front().second.transform(rad->get_center());
            dxf.write(&DL_Dxf::writeCircle, center.x, center.y, 0, rad->get_radius());
        }
        else {
            for (auto &[node, edge] : path) {
                const auto p_cur = node.get_pt_for_edge(edge);
                const auto p_next = p_cur == 1 ? 2 : 1;
                if (auto line = dynamic_cast<const EntityLine2D *>(&edge.entity)) {
                    const auto p1 = edge.transform(line->get_point_in_workplane(p_cur));
                    const auto p2 = edge.transform(line->get_point_in_workplane(p_next));

                    dxf.write(&DL_Dxf::writeLine, p1.x, p1.y, 0, p2.x, p2.y, 0);
                }
                else if (auto arc = dynamic_cast<const EntityArc2D *>(&edge.entity)) {
                    const auto center = edge.transform(arc->m_center);
                    const auto v1 = edge.transform(arc->get_point_in_workplane(p_cur)) - center;
                    const auto v2 = edge.transform(arc->get_point_in_workplane(p_next)) - center;
                    const auto a1 = glm::degrees(atan2(v1.y, v1.x));
                    const auto a2 = glm::degrees(atan2(v2.y, v2.x));
                    if (p_cur == 1)
                        dxf.write(&DL_Dxf::writeArc, center.x, center.y, 0, arc->get_radius(), a1, a2);
                    else
                        dxf.write(&DL_Dxf::writeArc, center.x, center.y, 0, arc->get_radius(), a2, a1);
                }
                else if (auto bez = dynamic_cast<const EntityBezier2D *>(&edge.entity)) {
                    const auto p1 = edge.transform(bez->get_point_in_workplane(p_cur));

                    const auto c1 = edge.transform(bez->get_point_in_workplane(p_next == 1 ? 4 : 3));
                    const auto c2 = edge.transform(bez->get_point_in_workplane(p_next == 1 ? 3 : 4));
                    const auto p2 = edge.transform(bez->get_point_in_workplane(p_next));

                    dxf.write(&DL_Dxf::writeSpline, 3, 0, 4, 0, 0);
                    dxf.write(&DL_Dxf::writeControlPoint, p1.x, p1.y, 0, 1);
                    dxf.write(&DL_Dxf::writeControlPoint, c1.x, c1.y, 0, 1);
                    dxf.write(&DL_Dxf::writeControlPoint, c2.x, c2.y, 0, 1);
                    dxf.write(&DL_Dxf::writeControlPoint, p2.x, p2.y, 0, 1);
                }
            }
        }
    }
}
} // namespace dune3d
