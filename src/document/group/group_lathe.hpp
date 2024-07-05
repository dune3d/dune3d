#pragma once
#include "group_sweep.hpp"
#include "igroup_pre_solve.hpp"
#include "document/entity/entity_and_point.hpp"
#include <glm/glm.hpp>

namespace dune3d {

class Document;
class SolidModel;

class GroupLathe : public GroupSweep, public IGroupPreSolve {
public:
    explicit GroupLathe(const UUID &uu);
    explicit GroupLathe(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::LATHE;
    Type get_type() const override
    {
        return s_type;
    }

    UUID m_normal;
    EntityAndPoint m_origin;

    std::optional<glm::dvec3> get_direction(const Document &doc) const;

    void update_solid_model(const Document &doc) override;

    UUID get_lathe_circle_uuid(const UUID &uu, unsigned int pt) const;

    void generate(Document &doc) const override;

    json serialize() const override;
    std::unique_ptr<Group> clone() const override;

    std::set<UUID> get_referenced_entities(const Document &doc) const override;

    std::set<UUID> get_required_entities(const Document &doc) const override;

    void pre_solve(Document &doc) const override;

private:
    enum class GenerateOrSolve { GENERATE, SOLVE };
    void generate_or_solve(Document &doc, GenerateOrSolve gen_or_solve) const;
};

} // namespace dune3d
