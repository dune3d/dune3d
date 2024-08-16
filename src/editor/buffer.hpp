#pragma once
#include <memory>
#include "util/uuid.hpp"
#include "util/badge.hpp"
#include <glm/glm.hpp>
#include <map>
#include <set>
#include <optional>

namespace dune3d {

class Entity;
class Constraint;
class Document;
class SelectableRef;

class Buffer {
public:
    enum class Operation { COPY, CUT };

    static std::unique_ptr<const Buffer> create(const Document &doc, const std::set<SelectableRef> &sel,
                                                Operation operation);

    static bool can_create(const Document &doc, const std::set<SelectableRef> &sel);

    std::map<UUID, std::unique_ptr<Entity>> m_entities;
    std::map<UUID, std::unique_ptr<Constraint>> m_constraints;
    UUID m_wrkpl;

    ~Buffer();

    Buffer(Badge<Buffer>, const Document &doc, const std::set<SelectableRef> &sel, Operation operation);

    std::optional<std::pair<glm::dvec2, glm::dvec2>> get_bbox() const;

private:
    void add(const Entity &entity);
    void add(const Constraint &constraint, Operation operation);
};
} // namespace dune3d
