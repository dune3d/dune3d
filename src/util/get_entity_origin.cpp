#include "get_entity_origin.hpp"
#include "document/entity/entity_visitor.hpp"
#include "document/entity/all_entities.hpp"
#include "document/document.hpp"

namespace dune3d {


namespace {
class OriginGetter : private EntityVisitor {
public:
    explicit OriginGetter(const Document &doc, const Entity &entity, unsigned int point) : m_doc(doc), m_point(point)
    {
        if (auto en_wrkpl = dynamic_cast<const IEntityInWorkplane *>(&entity))
            m_wrkpl = &doc.get_entity<EntityWorkplane>(en_wrkpl->get_workplane());
        entity.accept(*this);
    }

    void visit(const EntityLine3D &en) override
    {
        m_origin = (en.m_p1 + en.m_p2) / 2.;
    }

    void visit(const EntityLine2D &en) override
    {
        m_origin = m_wrkpl->transform((en.m_p1 + en.m_p2) / 2.);
    }

    void visit(const EntityArc2D &en) override
    {
        m_origin = m_wrkpl->transform(en.m_center);
    }

    void visit(const EntityArc3D &en) override
    {
        m_origin = en.m_center;
    }

    void visit(const EntityCircle2D &en) override
    {
        m_origin = m_wrkpl->transform(en.m_center);
    }

    void visit(const EntityCircle3D &en) override
    {
        m_origin = en.m_center;
    }

    void visit(const EntityWorkplane &en) override
    {
        m_origin = en.m_origin;
    }

    void visit(const EntitySTEP &en) override
    {
        m_origin = en.m_origin;
    }

    void visit(const EntityPoint2D &en) override
    {
        m_origin = m_wrkpl->transform(en.m_p);
    }

    void visit(const EntityDocument &en) override
    {
        m_origin = en.m_origin;
    }

    void visit(const EntityBezier2D &en) override
    {
        m_origin = m_wrkpl->transform(en.get_interpolated(.5));
    }
    
    void visit(const EntityBezier3D &en) override
    {
        m_origin = en.get_interpolated(.5);
    }
    
    void visit(const EntityCluster &en) override
    {
        m_origin = m_wrkpl->transform(en.m_origin);
    }
    
    void visit(const EntityText &en) override
    {
        m_origin = m_wrkpl->transform(en.m_origin);
    }
    
    void visit(const EntityPicture &en) override
    {
        m_origin = m_wrkpl->transform(en.m_origin);
    }

    const auto &get_origin() const
    {
        return m_origin;
    }

private:
    const Document &m_doc;
    const unsigned int m_point;
    const EntityWorkplane *m_wrkpl;
    glm::dvec3 m_origin;
};
} // namespace

glm::dvec3 get_entity_origin(const Document &doc, const EntityAndPoint &enp)
{
    auto &entity = doc.get_entity(enp.entity);
    if (entity.is_valid_point(enp.point))
        return entity.get_point(enp.point, doc);

    OriginGetter org{doc, entity, enp.point};

    return org.get_origin();
}

} // namespace dune3d
