#include "dxf_importer.hpp"
#include "document/document.hpp"
#include "document/group/group.hpp"
#include "document/entity/entity_workplane.hpp"
#include "document/entity/entity_line2d.hpp"
#include "document/entity/entity_bezier2d.hpp"
#include "document/entity/entity_circle2d.hpp"
#include "document/entity/entity_arc2d.hpp"
#include "dxflib/dl_creationadapter.h"
#include "dxflib/dl_dxf.h"
#include "util/fs_util.hpp"

namespace dune3d {


class DXFAdapter : public DL_CreationAdapter {
public:
    DXFAdapter(DXFImporter &importer) : m_importer(importer)
    {
    }

private:
    DXFImporter &m_importer;
    EntityBezier2D *m_bezier = nullptr;
    unsigned int m_bezier_point = 0;

    template <typename T> T &add_entity()
    {
        auto &en = m_importer.m_doc.add_entity<T>(UUID::random());
        en.m_wrkpl = m_importer.m_wrkpl.m_uuid;
        en.m_group = m_importer.m_group.m_uuid;
        m_importer.m_entities.insert(&en);
        return en;
    }

    void addLine(const DL_LineData &d) override
    {
        auto &line = add_entity<EntityLine2D>();
        line.m_p1 = {d.x1, d.y1};
        line.m_p2 = {d.x2, d.y2};
    }

    void addSpline(const DL_SplineData &d) override
    {
        if (d.degree != 3)
            return;
        if (d.nControl != 4)
            return;
        m_bezier = &add_entity<EntityBezier2D>();
        m_bezier_point = 0;
    }

    void addControlPoint(const DL_ControlPointData &d) override
    {
        if (!m_bezier)
            return;
        glm::dvec2 p{d.x, d.y};
        switch (m_bezier_point) {
        case 0:
            m_bezier->m_p1 = p;
            break;
        case 1:
            m_bezier->m_c1 = p;
            break;
        case 2:
            m_bezier->m_c2 = p;
            break;
        case 3:
            m_bezier->m_p2 = p;
            m_bezier = nullptr;
            break;
        }
        m_bezier_point++;
    }

    glm::dvec2 m_last_vertex;
    glm::dvec2 m_first_vertex;
    unsigned int m_poly_vertex = 0;
    unsigned int m_poly_vertices = 0;
    bool m_poly_is_closed = false;
    void addPolyline(const DL_PolylineData &d) override
    {
        m_poly_vertex = d.number;
        m_poly_vertices = d.number;
        m_last_vertex = {NAN, NAN};
        m_poly_is_closed = d.flags & 1;
    }


    void addVertex(const DL_VertexData &d) override
    {
        if (m_poly_vertex)
            m_poly_vertex--;
        else
            return;
        glm::dvec2 p{d.x, d.y};
        if (m_poly_vertex + 1 == m_poly_vertices)
            m_first_vertex = p;
        if (!isnan(m_last_vertex.x)) {
            auto &line = add_entity<EntityLine2D>();
            line.m_p1 = m_last_vertex;
            line.m_p2 = p;
        }
        if (m_poly_vertex == 0 && m_poly_is_closed) {
            auto &line = add_entity<EntityLine2D>();
            line.m_p1 = p;
            line.m_p2 = m_first_vertex;
        }

        m_last_vertex = p;
    }

    void addArc(const DL_ArcData &d) override
    {
        auto &arc = add_entity<EntityArc2D>();
        arc.m_center = {d.cx, d.cy};
        auto phi1 = glm::radians(d.angle1);
        auto phi2 = glm::radians(d.angle2);
        arc.m_from = glm::dvec2(d.cx + d.radius * cos(phi1), d.cy + d.radius * sin(phi1));
        arc.m_to = glm::dvec2(d.cx + d.radius * cos(phi2), d.cy + d.radius * sin(phi2));
    }

    void addCircle(const DL_CircleData &d) override
    {
        auto &circle = add_entity<EntityCircle2D>();
        circle.m_center = {d.cx, d.cy};
        circle.m_radius = {d.radius};
    }
};

DXFImporter::DXFImporter(Document &doc, const UUID &uu_group, const UUID &uu_wrkpl)
    : m_doc(doc), m_group(doc.get_group(uu_group)), m_wrkpl(doc.get_entity<EntityWorkplane>(uu_wrkpl))
{
}


bool DXFImporter::import(const std::filesystem::path &filename)
{
    DXFAdapter adapter(*this);
    DL_Dxf dxf;
    if (!dxf.in(path_to_string(filename), &adapter)) {
        std::cout << "import error" << std::endl;
        return false;
    }
    return true;
}
} // namespace dune3d
