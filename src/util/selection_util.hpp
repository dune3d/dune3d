#pragma once
#include "uuid.hpp"
#include <optional>
#include <set>
#include "document/entity/entity.hpp"
#include "document/entity/entity_and_point.hpp"

namespace dune3d {

class Document;
class SelectableRef;

struct TwoPoints {
    EntityAndPoint point1;
    EntityAndPoint point2;
};

std::optional<TwoPoints> two_points_from_selection(const Document &doc, const std::set<SelectableRef> &sel);

std::optional<EntityAndPoint> entity_and_point_from_hover_selection(const Document &doc,
                                                                    const std::optional<SelectableRef> &sel);

struct LineAndPoint {
    UUID line;
    EntityAndPoint point;

    enum class AllowSameEntity { YES, NO };
};

std::optional<LineAndPoint>
line_and_point_from_selection(const Document &doc, const std::set<SelectableRef> &sel,
                              LineAndPoint::AllowSameEntity allow_same_entity = LineAndPoint::AllowSameEntity::NO);
std::optional<LineAndPoint>
circle_and_point_from_selection(const Document &doc, const std::set<SelectableRef> &sel,
                                LineAndPoint::AllowSameEntity allow_same_entity = LineAndPoint::AllowSameEntity::NO);
std::optional<UUID> entity_from_selection(const Document &doc, const std::set<SelectableRef> &sel);
std::optional<UUID> entity_from_selection(const Document &doc, const std::set<SelectableRef> &sel, Entity::Type type);
} // namespace dune3d
