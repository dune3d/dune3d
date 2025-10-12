#pragma once

#include "constraintt.hpp"
#include "iconstraint_datum.hpp"
#include "iconstraint_movable.hpp"
#include "iconstraint_workplane.hpp"
#include <glm/glm.hpp>

namespace dune3d {

class Document;

class ConstraintLengthRatio : public ConstraintT<ConstraintLengthRatio>,
                              public IConstraintWorkplane,
                              public IConstraintDatum,
                              public IConstraintMovable {
public:
    explicit ConstraintLengthRatio(const UUID &uu);
    explicit ConstraintLengthRatio(const UUID &uu, const json &j);

    static constexpr Type s_type = Type::LENGTH_RATIO;
    static constexpr double s_min_ratio = 1e-6;
    static constexpr double s_max_ratio = 1e6;

    json serialize() const override;

    void accept(ConstraintVisitor &visitor) const override;

    const UUID &get_workplane(const Document &doc) const override
    {
        return m_wrkpl;
    }

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

    double get_datum() const override
    {
        return m_ratio;
    }
    void set_datum(double d) override
    {
        m_ratio = d;
    }
    double get_display_datum(const Document &doc) const override;
    double measure_datum(const Document &doc) const override;
    std::pair<double, double> get_datum_range() const override
    {
        return {s_min_ratio, s_max_ratio};
    }
    std::string get_datum_name() const override
    {
        return "Length ratio";
    }
    DatumUnit get_datum_unit() const override;
    std::string format_datum(double datum) const override;

    bool is_measurement() const override
    {
        return m_measurement;
    }
    void set_is_measurement(bool is_measurement) override
    {
        m_measurement = is_measurement;
    }

    static double measure_entity_length(const Document &doc, const UUID &entity);
    double measure_ratio(const Document &doc) const;

    UUID m_entity1;
    UUID m_entity2;
    double m_ratio = 1.0;
    UUID m_wrkpl;
    bool m_measurement = false;
    glm::dvec3 m_offset = {0.0, 0.0, 0.0};

    constexpr static auto s_referenced_entities_and_points_tuple = std::make_tuple(
            &ConstraintLengthRatio::m_entity1, &ConstraintLengthRatio::m_entity2, &ConstraintLengthRatio::m_wrkpl);
};

} // namespace dune3d