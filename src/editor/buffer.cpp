#include "buffer.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/entity/ientity_in_workplane.hpp"
#include "document/entity/ientity_bounding_box2d.hpp"
#include "document/constraint/constraint.hpp"
#include "canvas/selectable_ref.hpp"
#include "util/bbox_accumulator.hpp"
#include <algorithm>

namespace dune3d {
std::unique_ptr<const Buffer> Buffer::create(const Document &doc, const std::set<SelectableRef> &sel,
                                             Operation operation)
{
    auto buf = std::make_unique<Buffer>(Badge<Buffer>{}, doc, sel, operation);
    if (buf->m_entities.empty())
        return nullptr;
    return buf;
}

Buffer::Buffer(Badge<Buffer>, const Document &doc, const std::set<SelectableRef> &sel, Operation operation)

{
    {
        std::map<UUID, unsigned int> wrkpl_count;
        for (const auto &sr : sel) {
            if (sr.is_entity()) {
                const auto &en = doc.get_entity(sr.item);
                if (auto en_wrkpl = dynamic_cast<const IEntityInWorkplane *>(&en))
                    wrkpl_count[en_wrkpl->get_workplane()]++;
                else if (en.of_type(Entity::Type::WORKPLANE))
                    wrkpl_count[en.m_uuid]++;
            }
        }
        if (wrkpl_count.empty())
            m_wrkpl = UUID();
        else
            m_wrkpl = std::ranges::max_element(wrkpl_count, {}, [](const auto &it) { return it.second; })->first;
    }
    for (const auto &sr : sel) {
        switch (sr.type) {
        case SelectableRef::Type::ENTITY:
            add(doc.get_entity(sr.item));
            break;

        case SelectableRef::Type::CONSTRAINT:
            add(doc.get_constraint(sr.item), operation);
            break;

        case SelectableRef::Type::SOLID_MODEL_EDGE:
        case SelectableRef::Type::DOCUMENT:
            break;
        }
    }

    for (const auto &[uu, en] : m_entities) {
        // add constraints referenced by entities
        const auto constraints = en->get_constraints(doc);
        for (const auto constraint : constraints) {
            // constraint only references entities from buffer?
            const auto entities = constraint->get_referenced_entities();
            const bool all_enties_are_from_buffer = std::ranges::all_of(
                    entities, [this](const auto &uu) { return m_entities.contains(uu) || m_wrkpl == uu; });
            if (operation == Operation::CUT || all_enties_are_from_buffer) {
                add(*constraint, operation);
            }
        }
    }
}

static bool entity_is_supported(const Entity &entity)
{
    if (entity.of_type(Entity::Type::WORKPLANE))
        return true;
    if (entity.m_kind != ItemKind::USER)
        return false;
    if (entity.of_type(Entity::Type::DOCUMENT))
        return false;
    return true;
}

void Buffer::add(const Entity &entity)
{
    if (m_entities.contains(entity.m_uuid))
        return;
    if (auto en_wrkpl = dynamic_cast<const IEntityInWorkplane *>(&entity)) {
        if (en_wrkpl->get_workplane() != m_wrkpl)
            return;
    }
    if (!entity_is_supported(entity))
        return;
    if (entity.of_type(EntityType::WORKPLANE) && entity.m_uuid != m_wrkpl)
        return;

    m_entities.emplace(entity.m_uuid, entity.clone());
}

void Buffer::add(const Constraint &constraint, Operation operation)
{
    if (m_constraints.contains(constraint.m_uuid))
        return;

    m_constraints.emplace(constraint.m_uuid, constraint.clone());
}

bool Buffer::can_create(const Document &doc, const std::set<SelectableRef> &sel)
{
    for (const auto &sr : sel) {
        if (sr.is_entity()) {
            const auto entity = doc.get_entity_ptr(sr.item);
            if (!entity)
                continue;
            if (!entity_is_supported(*entity))
                continue;
            return true;
        }
    }
    return false;
}

std::optional<std::pair<glm::dvec2, glm::dvec2>> Buffer::get_bbox() const
{
    BBoxAccumulator<glm::dvec2> acc;
    for (const auto &[uu, en] : m_entities) {
        if (auto bb = dynamic_cast<const IEntityBoundingBox2D *>(en.get()))
            acc.accumulate(bb->get_bbox());
    }
    return acc.get();
}

Buffer::~Buffer() = default;

} // namespace dune3d
