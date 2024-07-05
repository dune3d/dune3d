#pragma once
#include "group_circular_sweep.hpp"
#include "igroup_pre_solve.hpp"
#include <glm/glm.hpp>

namespace dune3d {

class Document;
class SolidModel;

class GroupLathe : public GroupCircularSweep, public IGroupPreSolve {
public:
    using GroupCircularSweep::GroupCircularSweep;

    static constexpr Type s_type = Type::LATHE;
    Type get_type() const override
    {
        return s_type;
    }

    void update_solid_model(const Document &doc) override;

    UUID get_lathe_circle_uuid(const UUID &uu, unsigned int pt) const;

    void generate(Document &doc) const override;

    std::unique_ptr<Group> clone() const override;

    void pre_solve(Document &doc) const override;

private:
    enum class GenerateOrSolve { GENERATE, SOLVE };
    void generate_or_solve(Document &doc, GenerateOrSolve gen_or_solve) const;
};

} // namespace dune3d
