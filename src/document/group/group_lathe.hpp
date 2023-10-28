#pragma once
#include "group_sweep.hpp"
#include <glm/glm.hpp>

namespace dune3d {

class Document;
class SolidModel;

class GroupLathe : public GroupSweep {
public:
    explicit GroupLathe(const UUID &uu);
    explicit GroupLathe(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::LATHE;
    Type get_type() const override
    {
        return s_type;
    }

    UUID m_normal;
    UUID m_origin;
    unsigned int m_origin_point;

    void update_solid_model(const Document &doc) override;

    UUID get_lathe_circle_uuid(const UUID &uu, unsigned int pt) const;

    void generate(Document &doc) const override;

    json serialize() const override;
    std::unique_ptr<Group> clone() const override;

    std::set<UUID> get_referenced_entities(const Document &doc) const override;

    std::set<UUID> get_required_entities(const Document &doc) const override;
};

} // namespace dune3d
