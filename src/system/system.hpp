#pragma once
#include <memory>
#include <map>
#include <mutex>
#include <functional>
#include "util/uuid.hpp"
#include "document/constraint/all_constraints_fwd.hpp"
#include "document/entity/all_entities_fwd.hpp"
#include "document/group/all_groups_fwd.hpp"
#include "document/entity/entity_visitor.hpp"
#include "document/constraint/constraint_visitor.hpp"
#include "document/entity/entity_and_point.hpp"


namespace SolveSpace {
class System;
class ExprVector;
class ExprQuaternion;
} // namespace SolveSpace

namespace dune3d {

class Document;

class System : private EntityVisitor, private ConstraintVisitor {
public:
    System(Document &doc, const UUID &group);

    bool solve();

    void update_document();

    void add_dragged(const UUID &entity, unsigned int point);

    ~System();

private:
    struct ParamRef {
        enum class Type { ENTITY, GROUP };
        Type type;
        UUID item;
        unsigned int point;
        unsigned int axis;
    };
    using EntityRef = EntityAndPoint;

    std::map<unsigned int, ParamRef> m_param_refs;
    std::map<unsigned int, EntityRef> m_entity_refs;
    std::map<EntityRef, unsigned int> m_entity_refs_r;

    std::map<unsigned int, UUID> m_constraint_refs;

    unsigned int get_entity_ref(const EntityRef &ref);

    uint32_t add_param(const UUID &group_uu, double value);
    uint32_t add_param(const UUID &group_uu, const UUID &entity, unsigned int point, unsigned int axis);

    void visit(const EntityLine3D &line) override;
    void visit(const EntityLine2D &line) override;
    void visit(const EntityArc2D &arc) override;
    void visit(const EntityCircle2D &arc) override;
    void visit(const EntityCircle3D &arc) override;
    void visit(const EntityArc3D &arc) override;
    void visit(const EntityWorkplane &wrkpl) override;
    void visit(const EntitySTEP &step) override;
    void visit(const EntityPoint2D &point) override;
    void visit(const ConstraintPointsCoincident &constraint) override;
    void visit(const ConstraintParallel &constraint) override;
    void visit(const ConstraintPointOnLine &constraint) override;
    void visit(const ConstraintPointOnCircle &constraint) override;
    void visit(const ConstraintEqualLength &constraint) override;
    void visit(const ConstraintEqualRadius &constraint) override;
    void visit(const ConstraintSameOrientation &constraint) override;
    void visit(const ConstraintHV &constraint) override;
    void visit(const ConstraintPointDistance &constraint) override;
    void visit(const ConstraintPointDistanceHV &constraint) override;
    void visit(const ConstraintWorkplaneNormal &constraint) override;
    void visit(const ConstraintMidpoint &constraint) override;
    void visit(const ConstraintDiameterRadius &constraint) override;
    void visit(const ConstraintArcLineTangent &constraint) override;
    void visit(const ConstraintArcArcTangent &constraint) override;
    void visit(const ConstraintLinePointsPerpendicular &constraint) override;
    void visit(const ConstraintLinesPerpendicular &constr) override;
    void visit(const ConstraintLinesAngle &constr) override;
    void add(const GroupExtrude &group);
    void add(const GroupLinearArray &group);
    void add(const GroupLathe &group);
    using CreateEq = std::function<void(const SolveSpace::ExprVector &exorig, const SolveSpace::ExprVector &exnew,
                                        unsigned int instance)>;
    using CreateEqN = std::function<void(const SolveSpace::ExprQuaternion &exorig,
                                         const SolveSpace::ExprQuaternion &exnew, unsigned int instance)>;
    void add_array(const GroupArray &group, CreateEq create_eq2, CreateEq create_eq3, CreateEqN create_eq_n,
                   unsigned int &eqi);
    std::unique_ptr<SolveSpace::System> m_sys;
    Document &m_doc;
    const UUID m_solve_group;
    std::lock_guard<std::mutex> m_lock;

    unsigned int n_constraint = 1;

    int get_group_index(const UUID &uu) const;
    int get_group_index(const Constraint &constraint) const;
    int get_group_index(const Entity &en) const;

    EntityRef get_entity_ref_for_parallel(const UUID &uu) const;
};

} // namespace dune3d
