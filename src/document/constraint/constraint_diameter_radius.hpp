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
    virtual void measure(const Document &doc) = 0;

    std::set<EntityAndPoint> get_referenced_entities_and_points() const override;

    double get_datum() const override
    {
        return m_distance;
    }

    void set_datum(double d) override
    {
        m_distance = d;
    }

    DatumUnit get_datum_unit() const override
    {
        return DatumUnit::MM;
    }

    std::pair<double, double> get_datum_range() const override
    {
        return {0, 1e3};
    }

    const UUID &get_workplane(const Document &doc) const override;

    void accept(ConstraintVisitor &visitor) const override;

protected:
    double measure_radius(const Document &doc);
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

    void measure(const Document &doc) override;

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

    void measure(const Document &doc) override;

    std::unique_ptr<Constraint> clone() const override;
};

} // namespace dune3d
