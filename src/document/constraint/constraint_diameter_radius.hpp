#pragma once
#include "constraint.hpp"
#include "iconstraint_datum.hpp"
#include "iconstraint_workplane.hpp"
#include "iconstraint_movable.hpp"
#include <glm/glm.hpp>

namespace dune3d {

class Entity;
class Document;

class ConstraintDiameterRadius : public Constraint,
                                 public IConstraintDatum,
                                 public IConstraintWorkplane,
                                 public IConstraintMovable {
public:
    explicit ConstraintDiameterRadius(const UUID &uu);
    explicit ConstraintDiameterRadius(const UUID &uu, const json &j);
    json serialize() const override;

    UUID m_entity;
    double m_distance = 1;
    glm::dvec2 m_offset = {1, 0};
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
        return glm::dvec3(m_offset, 0);
    }

    void set_offset(const glm::dvec3 &offset) override
    {
        m_offset = offset;
    }

    bool offset_is_in_workplane() const override
    {
        return true;
    }

    virtual double get_diameter() const = 0;
    void measure(const Document &doc);
    virtual double measure_distance(const Document &doc) const = 0;

    std::set<EntityAndPoint> get_referenced_entities_and_points() const override;

    double get_datum() const override
    {
        return m_distance;
    }

    void set_datum(double d) override
    {
        m_distance = d;
    }

    double measure_datum(const Document &doc) const override;

    DatumUnit get_datum_unit() const override
    {
        return DatumUnit::MM;
    }

    std::pair<double, double> get_datum_range() const override
    {
        return {0, 1e6};
    }

    const UUID &get_workplane(const Document &doc) const override;

    double get_display_distance(const Document &doc) const;

    void accept(ConstraintVisitor &visitor) const override;

    bool replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point) override;

    constexpr static auto s_referenced_entities_and_points_tuple = std::make_tuple(&ConstraintDiameterRadius::m_entity);

protected:
    double measure_radius(const Document &doc) const;
};

class ConstraintDiameter : public ConstraintDiameterRadius {
public:
    using ConstraintDiameterRadius::ConstraintDiameterRadius;
    static constexpr Type s_type = Type::DIAMETER;
    Type get_type() const override
    {
        return s_type;
    }

    double get_diameter() const override
    {
        return m_distance;
    }

    std::string get_datum_name() const override
    {
        return "Diameter";
    }

    double measure_distance(const Document &doc) const override;

    std::unique_ptr<Constraint> clone() const override;
};

class ConstraintRadius : public ConstraintDiameterRadius {
public:
    using ConstraintDiameterRadius::ConstraintDiameterRadius;
    static constexpr Type s_type = Type::RADIUS;
    Type get_type() const override
    {
        return s_type;
    }

    double get_diameter() const override
    {
        return m_distance * 2;
    }

    std::string get_datum_name() const override
    {
        return "Radius";
    }

    double measure_distance(const Document &doc) const override;

    std::unique_ptr<Constraint> clone() const override;
};

} // namespace dune3d
