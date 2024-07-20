#pragma once
#include <filesystem>
#include <set>
#include "util/uuid.hpp"

namespace dune3d {

class Document;
class Group;
class EntityWorkplane;
class Entity;

class DXFImporter {
public:
    friend class DXFAdapter;
    DXFImporter(Document &doc, const UUID &uu_group, const UUID &uu_wrkpl);
    bool import(const std::filesystem::path &filename);

    const auto &get_entities() const
    {
        return m_entities;
    }

private:
    Document &m_doc;
    Group &m_group;
    EntityWorkplane &m_wrkpl;

    std::set<Entity *> m_entities;
};
} // namespace dune3d
