#pragma once
#include "constraint.hpp"
#include "iconstraint_datum.hpp"
#include "iconstraint_workplane.hpp"
#include "iconstraint_movable.hpp"
#include <glm/glm.hpp>

namespace dune3d {

class Entity;

class ConstraintAngleBase : public Constraint, public IConstraintWorkplane {
public:
    explicit ConstraintAngleBase(const UUID &uu);
    explicit ConstraintAngleBase(const UUID &uu, const json &j);

    json serialize() const override;

    UUID m_entity1;
    UUID m_entity2;
    UUID m_wrkpl;

    const UUID &get_workplane(const Document &doc) const override
    {
        return m_wrkpl;
    }

    bool replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point) override;
    std::set<EntityAndPoint> get_referenced_entities_and_points() const override;

    constexpr static auto s_referenced_entities_and_points_tuple = std::make_tuple(
            &ConstraintAngleBase::m_entity1, &ConstraintAngleBase::m_entity2, &ConstraintAngleBase::m_wrkpl);
};

class ConstraintLinesPerpendicular : public ConstraintAngleBase {
public:
    using ConstraintAngleBase::ConstraintAngleBase;

    static constexpr Type s_type = Type::LINES_PERPENDICULAR;
    Type get_type() const override
    {
        return s_type;
    }

    std::unique_ptr<Constraint> clone() const override;
    void accept(ConstraintVisitor &visitor) const override;
};

class ConstraintLinesAngle : public ConstraintAngleBase, public IConstraintDatum, public IConstraintMovable {
public:
    using ConstraintAngleBase::ConstraintAngleBase;
    explicit ConstraintLinesAngle(const UUID &uu, const json &j);
    json serialize() const override;


    static constexpr Type s_type = Type::LINES_ANGLE;
    Type get_type() const override
    {
        return s_type;
    }

    double m_angle = 0;
    bool m_negative = false;
    glm::dvec3 m_offset = {0, 0, 0};
    glm::dvec3 get_origin(const Document &doc) const override;

    bool m_measurement = false;

    bool is_measurement() const override
    {
        return m_measurement;
    }
    void set_is_measurement(bool is_measurement) override
    {
        m_measurement = is_measurement;
    }

    glm::dvec3 get_offset() const override
    {
        return m_offset;
    }

    void set_offset(const glm::dvec3 &offset) override
    {
        m_offset = offset;
    }

    bool offset_is_in_workplane() const override
    {
        return false;
    }

    double get_datum() const override
    {
        return m_angle;
    }

    void set_datum(double d) override
    {
        m_angle = d;
    }

    double measure_datum(const Document &doc) const override;

    DatumUnit get_datum_unit() const override
    {
        return DatumUnit::DEGREE;
    }

    std::string format_datum(double datum) const override;

    std::pair<double, double> get_datum_range() const override
    {
        return {0, 360};
    }

    std::string get_datum_name() const override
    {
        return "Angle";
    }

    struct Vectors {
        glm::dvec3 l1p1;
        glm::dvec3 l2p1;
        glm::dvec3 l1v;
        glm::dvec3 l2v;
        glm::dvec3 n;
        glm::dvec3 u;
        glm::dvec3 v;
    };
    Vectors get_vectors(const Document &doc) const;

    std::unique_ptr<Constraint> clone() const override;
    void accept(ConstraintVisitor &visitor) const override;

    double get_display_datum(const Document &doc) const override;
};

} // namespace dune3d
