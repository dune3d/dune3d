#pragma once
#include "all_entities_fwd.hpp"

namespace dune3d {
class EntityVisitor {
public:
    virtual void visit(const EntityLine3D &en) = 0;
    virtual void visit(const EntityLine2D &en) = 0;
    virtual void visit(const EntityArc2D &en) = 0;
    virtual void visit(const EntityArc3D &en) = 0;
    virtual void visit(const EntityCircle2D &en) = 0;
    virtual void visit(const EntityCircle3D &en) = 0;
    virtual void visit(const EntityWorkplane &en) = 0;
    virtual void visit(const EntitySTEP &en) = 0;
    virtual void visit(const EntityPoint2D &en) = 0;
    virtual void visit(const EntityDocument &en) = 0;
    virtual void visit(const EntityBezier2D &en) = 0;
    virtual void visit(const EntityBezier3D &en) = 0;
    virtual void visit(const EntityCluster &en) = 0;
    virtual void visit(const EntityText &en) = 0;
};
} // namespace dune3d
