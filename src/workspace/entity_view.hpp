#pragma once
#include <memory>

namespace dune3d {

enum class EntityType;

class EntityView {
public:
    static std::unique_ptr<EntityView> create_for_type(EntityType type);
    virtual EntityType get_type() const = 0;
    virtual ~EntityView();
};

class EntityViewSTEP : public EntityView {
public:
    enum class Display { SOLID, WIREFRAME, OFF };
    Display display = Display::SOLID;
    EntityType get_type() const override;
};

} // namespace dune3d
