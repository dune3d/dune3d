#pragma once
#include "canvas/face.hpp"
#include <memory>
#include <filesystem>
#include <vector>
#include <map>
#include <glm/glm.hpp>
#include "group/all_groups_fwd.hpp"

namespace dune3d {

class Document;
class SolidModelOcc;
class Group;

class SolidModel {
public:
    face::Faces m_faces;
    std::map<unsigned int, std::vector<glm::dvec3>> m_edges;

    static std::shared_ptr<const SolidModel> create(const Document &doc, GroupExtrude &group);
    static std::shared_ptr<const SolidModel> create(const Document &doc, GroupFillet &group);
    static std::shared_ptr<const SolidModel> create(const Document &doc, GroupChamfer &group);
    static std::shared_ptr<const SolidModel> create(const Document &doc, GroupLathe &group);
    virtual void export_stl(const std::filesystem::path &path) const = 0;
    virtual void export_step(const std::filesystem::path &path) const = 0;

    virtual ~SolidModel();

    static const SolidModel *get_last_solid_model(const Document &doc, const Group &group);
    static const IGroupSolidModel *get_last_solid_model_group(const Document &doc, const Group &group);
};

} // namespace dune3d
