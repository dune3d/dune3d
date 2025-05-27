#pragma once
#include <optional>
#include <set>
#include <array>
#include <list>
#include "canvas/selectable_ref.hpp"

namespace dune3d {

class Document;
class SelectableRef;
enum class EntityType;
class IDocumentProvider;
class ConstraintPointsCoincident;

std::set<SelectableRef> filter_selection(const std::set<SelectableRef> &sel, SelectableRef::Type type);
std::set<SelectableRef> entities_from_selection(const std::set<SelectableRef> &sel);

struct TwoPoints {
    EntityAndPoint point1;
    EntityAndPoint point2;

    std::set<EntityAndPoint> get_enps() const;
    std::tuple<EntityAndPoint, EntityAndPoint> get_enps_as_tuple() const;
};

std::optional<TwoPoints> two_points_from_selection(const Document &doc, const std::set<SelectableRef> &sel);

std::optional<EntityAndPoint> entity_and_point_from_hover_selection(const Document &doc,
                                                                    const std::optional<SelectableRef> &sel);

struct LineAndPoint {
    UUID line;
    EntityAndPoint point;

    enum class AllowSameEntity { YES, NO };

    std::set<EntityAndPoint> get_enps() const;
    std::tuple<EntityAndPoint, EntityAndPoint> get_enps_as_tuple() const;
};


std::optional<LineAndPoint> entity_and_point_from_selection(const Document &doc, const std::set<SelectableRef> &sel,
                                                            const std::set<EntityType> &types,
                                                            LineAndPoint::AllowSameEntity allow_same_entity);
std::optional<LineAndPoint>
line_and_point_from_selection(const Document &doc, const std::set<SelectableRef> &sel,
                              LineAndPoint::AllowSameEntity allow_same_entity = LineAndPoint::AllowSameEntity::NO);
std::optional<LineAndPoint>
bezier_and_point_from_selection(const Document &doc, const std::set<SelectableRef> &sel,
                                LineAndPoint::AllowSameEntity allow_same_entity = LineAndPoint::AllowSameEntity::NO);
std::optional<LineAndPoint>
circle_and_point_from_selection(const Document &doc, const std::set<SelectableRef> &sel,
                                LineAndPoint::AllowSameEntity allow_same_entity = LineAndPoint::AllowSameEntity::NO);
std::optional<EntityAndPoint> point_from_selection(const Document &doc, const std::set<SelectableRef> &sel);
std::optional<EntityAndPoint> point_from_selection(const Document &doc, const std::set<SelectableRef> &sel,
                                                   EntityType type);


struct LinesAndPoint {
    std::array<UUID, 2> lines;
    EntityAndPoint point;

    std::set<EntityAndPoint> get_enps() const;
    std::tuple<EntityAndPoint, EntityAndPoint, EntityAndPoint> get_enps_as_tuple() const;
};

std::optional<LinesAndPoint> lines_and_point_from_selection(const Document &doc, const std::set<SelectableRef> &sel);
std::optional<UUID> document_from_selection(const std::set<SelectableRef> &sel);

const ConstraintPointsCoincident *constraint_points_coincident_from_selection(const Document &doc,
                                                                              const std::set<SelectableRef> &sel,
                                                                              const std::set<EntityType> &types);

std::list<UUID> entities_from_selection(const Document &doc, const std::set<SelectableRef> &sel,
                                        const std::set<EntityType> &types);


std::string get_selectable_ref_description(IDocumentProvider &prv, const UUID &current_doc, const SelectableRef &sr);

} // namespace dune3d
