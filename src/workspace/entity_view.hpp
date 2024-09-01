#pragma once
#include <memory>
#include "nlohmann/json_fwd.hpp"

namespace dune3d {

using json = nlohmann::json;

enum class EntityType;

class EntityView {
public:
    static std::unique_ptr<EntityView> create_for_type(EntityType type);
    static std::unique_ptr<EntityView> new_from_json(const json &j);
    virtual EntityType get_type() const = 0;
    virtual std::unique_ptr<EntityView> clone() const = 0;
    virtual json serialize() const;
    virtual ~EntityView();
};

class EntityViewSTEP : public EntityView {
public:
    EntityViewSTEP();
    explicit EntityViewSTEP(const json &j);
    enum class Display { SOLID, OFF, WIREFRAME, SOLID_WIREFRAME };
    Display m_display = Display::SOLID;
    EntityType get_type() const override;
    std::unique_ptr<EntityView> clone() const override;
    json serialize() const override;
};

} // namespace dune3d
