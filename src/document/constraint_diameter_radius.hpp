#pragma once
#include "constraint.hpp"
#include <glm/glm.hpp>

namespace dune3d {

class Entity;
class Document;

class ConstraintDiameterRadius : public Constraint {
public:
    explicit ConstraintDiameterRadius(const UUID &uu);
    explicit ConstraintDiameterRadius(const UUID &uu, const json &j);
    json serialize() const override;

    UUID m_entity;
    double m_distance = 1;
    glm::dvec2 m_offset = {1, 0};

    virtual double get_diameter() const = 0;
    virtual void measure(const Document &doc) = 0;

    std::set<UUID> get_referenced_entities() const override;

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

    void measure(const Document &doc) override;

    std::unique_ptr<Constraint> clone() const override;
};

} // namespace dune3d
