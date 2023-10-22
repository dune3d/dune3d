#include "entity_step.hpp"
#include "nlohmann/json.hpp"
#include "util/glm_util.hpp"
#include "import_step/step_import_manager.hpp"
#include "entity_visitor.hpp"

namespace dune3d {
EntitySTEP::EntitySTEP(const UUID &uu) : Entity(uu), m_normal(glm::quat_identity<double, glm::defaultp>())
{
}

EntitySTEP::EntitySTEP(const UUID &uu, const json &j, const std::filesystem::path &containing_dir)
    : Entity(uu, j), m_origin(j.at("origin").get<glm::dvec3>()), m_normal(j.at("normal").get<glm::dquat>()),
      m_path(j.at("path").get<std::filesystem::path>())
{
    m_imported = STEPImportManager::get().import_step(get_path(containing_dir));
    for (const auto &[k, v] : j.at("anchors").items()) {
        m_anchors.emplace(std::stoi(k), v.get<glm::dvec3>());
        m_anchors_transformed.emplace(std::stoi(k), transform(v.get<glm::dvec3>()));
    }
}

std::filesystem::path EntitySTEP::get_path(const std::filesystem::path &containing_dir) const
{
    if (m_path.is_absolute())
        return m_path;
    else
        return containing_dir / m_path;
}

json EntitySTEP::serialize() const
{
    json j = Entity::serialize();
    j["origin"] = m_origin;
    j["normal"] = m_normal;
    j["path"] = m_path;
    // j["anchors"] = m_anchors;
    {
        json o = json::object();
        for (const auto &[k, v] : m_anchors) {
            o[std::to_string(k)] = v;
        }
        j["anchors"] = o;
    }
    return j;
}


glm::dvec3 EntitySTEP::transform(glm::dvec3 p) const
{
    return glm::rotate(m_normal, p) + m_origin;
}

double EntitySTEP::get_param(unsigned int point, unsigned int axis) const
{
    if (point == 1) {
        return m_origin[axis];
    }
    else if (point == 2) {
        return m_normal[axis];
    }
    else if (m_anchors_transformed.contains(point)) {
        return m_anchors_transformed.at(point)[axis];
    }
    return NAN;
}

void EntitySTEP::set_param(unsigned int point, unsigned int axis, double value)
{
    if (point == 1) {
        m_origin[axis] = value;
    }
    else if (point == 2) {
        m_normal[axis] = value;
    }
    else if (m_anchors_transformed.contains(point)) {
        m_anchors_transformed.at(point)[axis] = value;
    }
}

std::unique_ptr<Entity> EntitySTEP::clone() const
{
    return std::make_unique<EntitySTEP>(*this);
}

glm::dvec3 EntitySTEP::get_point(unsigned int point, const Document &doc) const
{
    if (point == 1)
        return m_origin;
    if (m_anchors_transformed.contains(point))
        return m_anchors_transformed.at(point);
    return {NAN, NAN, NAN};
}

bool EntitySTEP::is_valid_point(unsigned int point) const
{
    return point == 1 || m_anchors.contains(point);
}

void EntitySTEP::add_anchor(unsigned int i, const glm::dvec3 &pt)
{
    m_anchors.emplace(i, pt);
    m_anchors_transformed.emplace(i, transform(pt));
}

void EntitySTEP::remove_anchor(unsigned int i)
{
    m_anchors.erase(i);
    m_anchors_transformed.erase(i);
}

void EntitySTEP::accept(EntityVisitor &visitor) const
{
    visitor.visit(*this);
}


} // namespace dune3d
