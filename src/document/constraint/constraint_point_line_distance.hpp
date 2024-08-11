#pragma once
#include "constraintt.hpp"
#include "document/entity/entity_and_point.hpp"
#include "iconstraint_datum.hpp"
#include "iconstraint_workplane.hpp"
#include "iconstraint_movable.hpp"
#include <glm/glm.hpp>

namespace dune3d {

class ConstraintPointLineDistance : public ConstraintT<ConstraintPointLineDistance>,
                                    public IConstraintDatum,
                                    public IConstraintWorkplane,
                                    public IConstraintMovable {
public:
    explicit ConstraintPointLineDistance(const UUID &uu);
    explicit ConstraintPointLineDistance(const UUID &uu, const json &j);
    json serialize() const override;

    static constexpr Type s_type = Type::POINT_LINE_DISTANCE;

    EntityAndPoint m_point;
    UUID m_line;

    UUID m_wrkpl;

    const UUID &get_workplane(const Document &doc) const override
    {
        return m_wrkpl;
    }

    double m_distance = 1;
    glm::dvec3 m_offset = {0, 0, 0};

    bool m_measurement = false;

    bool is_measurement() const override
    {
        return m_measurement;
    }

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

    constexpr static auto s_referenced_entities_and_points_tuple =
            std::make_tuple(&ConstraintPointLineDistance::m_point, &ConstraintPointLineDistance::m_line,
                            &ConstraintPointLineDistance::m_wrkpl);

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
        if (m_wrkpl)
            return {-1e6, 1e6};
        else
            return {0, 1e6};
    }

    double measure_distance(const Document &doc) const;
    double get_display_distance(const Document &doc) const;
};


} // namespace dune3d
