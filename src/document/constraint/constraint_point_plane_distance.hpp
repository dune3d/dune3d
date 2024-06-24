#pragma once
#include "constraint.hpp"
#include "document/entity/entity_and_point.hpp"
#include "iconstraint_datum.hpp"
#include "iconstraint_movable.hpp"
#include <glm/glm.hpp>

namespace dune3d {

class Entity;

class ConstraintPointPlaneDistance : public Constraint, public IConstraintDatum, public IConstraintMovable {
public:
    explicit ConstraintPointPlaneDistance(const UUID &uu);
    explicit ConstraintPointPlaneDistance(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::POINT_PLANE_DISTANCE;
    Type get_type() const override
    {
        return s_type;
    }
    json serialize() const override;
    std::unique_ptr<Constraint> clone() const override;

    double m_distance = 1;
    glm::dvec3 m_offset = {0, 0, 0};

    bool m_measurement = false;

    bool is_measurement() const override
    {
        return m_measurement;
    }

    double measure_distance(const Document &doc) const;

    glm::dvec3 get_projected(const Document &doc) const;
    glm::dvec3 get_origin(const Document &doc) const override;

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

    std::string get_datum_name() const override
    {
        return "Distance";
    }

    double get_datum() const override
    {
        return std::abs(m_distance);
    }

    void set_datum(double d) override
    {
        m_distance = d * (m_distance >= 0 ? 1 : -1);
    }

    DatumUnit get_datum_unit() const override
    {
        return DatumUnit::MM;
    }

    std::pair<double, double> get_datum_range() const override
    {
        return {-1e6, 1e6};
    }

    double get_display_distance(const Document &doc) const;

    EntityAndPoint m_point;
    UUID m_line1;
    UUID m_line2;

    std::set<EntityAndPoint> get_referenced_entities_and_points() const override;

    void accept(ConstraintVisitor &visitor) const override;

    bool replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point) override;
};

} // namespace dune3d
