#pragma once
#include "constraint.hpp"
#include "document/entity/entity_and_point.hpp"
#include "iconstraint_datum.hpp"
#include "iconstraint_workplane.hpp"
#include "iconstraint_movable.hpp"
#include <glm/glm.hpp>

namespace dune3d {

class ConstraintPointDistanceBase : public Constraint,
                                    public IConstraintDatum,
                                    public IConstraintWorkplane,
                                    public IConstraintMovable {
public:
    explicit ConstraintPointDistanceBase(const UUID &uu);
    explicit ConstraintPointDistanceBase(const UUID &uu, const json &j);
    json serialize() const override;

    EntityAndPoint m_entity1;
    EntityAndPoint m_entity2;

    UUID m_wrkpl;

    bool m_measurement = false;

    bool is_measurement() const override
    {
        return m_measurement;
    }

    double m_distance = 1;
    glm::dvec3 m_offset = {0, 0, 0};
    glm::dvec3 get_origin(const Document &doc) const override;

    double get_display_distance(const Document &doc) const;

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

    virtual double measure_distance(const Document &doc) const = 0;


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

    const UUID &get_workplane(const Document &doc) const override
    {
        return m_wrkpl;
    }

    void flip();

    std::set<EntityAndPoint> get_referenced_entities_and_points() const override;
    bool replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point) override;
    constexpr static auto s_referenced_entities_and_points_tuple =
            std::make_tuple(&ConstraintPointDistanceBase::m_entity1, &ConstraintPointDistanceBase::m_entity2,
                            &ConstraintPointDistanceBase::m_wrkpl);

protected:
    glm::dvec3 get_distance_vector(const Document &doc) const;
};

class ConstraintPointDistance : public ConstraintPointDistanceBase {
public:
    using ConstraintPointDistanceBase::ConstraintPointDistanceBase;

    static constexpr Type s_type = Type::POINT_DISTANCE;
    Type get_type() const override
    {
        return s_type;
    }

    double measure_distance(const Document &doc) const override;
    std::string get_datum_name() const override
    {
        return "Distance";
    }

    std::pair<double, double> get_datum_range() const override
    {
        return {0, 1e6};
    }

    std::unique_ptr<Constraint> clone() const override;

    void accept(ConstraintVisitor &visitor) const override;
};


} // namespace dune3d
