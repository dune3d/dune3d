#pragma once
#include "util/uuid.hpp"

namespace dune3d {

class EntityView;

class IDocumentView {
public:
    virtual bool document_is_visible() const = 0;
    virtual bool body_is_visible(const UUID &uu) const = 0;
    virtual bool body_solid_model_is_visible(const UUID &uu) const = 0;
    virtual bool group_is_visible(const UUID &uu) const = 0;
    virtual const EntityView *get_entity_view(const UUID &uu) const = 0;
    virtual bool construction_entities_from_previous_groups_are_visible() const = 0;
    virtual bool hide_irrelevant_workplanes() const = 0;
};

} // namespace dune3d
