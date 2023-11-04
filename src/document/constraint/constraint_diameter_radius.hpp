#pragma once
#include "constraint.hpp"
#include "iconstraint_datum.hpp"
#include <glm/glm.hpp>

namespace dune3d {

class Entity;
class Document;

class ConstraintDiameterRadius : public Constraint, public IConstraintDatum {
public:
    explicit ConstraintDiameterRadius(const UUID &uu);
    explicit ConstraintDiameterRadius(const UUID &uu, const json &j);
    json serialize() const override;

    UUID m_entity;
    double m_distance = 1;
    glm::dvec2 m_offset = {1, 0};
    glm::dvec2 get_origin(const Document &doc) const;

    virtual double get_diameter() const = 0;
    virtual void measure(const Document &doc) = 0;

    std::set<UUID> get_referenced_entities() const override;

    double get_datum() const override
    {
        return m_distance;
    }

    void set_datum(double d) override
    {
        m_distance = d;
    }

    bool is_movable() const override
    {
        return true;
    }

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
