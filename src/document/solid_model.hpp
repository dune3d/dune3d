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
class IGroupSolidModel;

class SolidModel {
public:
    face::Faces m_faces;
    std::map<unsigned int, std::vector<glm::dvec3>> m_edges;

    static std::shared_ptr<const SolidModel> create(const Document &doc, GroupExtrude &group);
    static std::shared_ptr<const SolidModel> create(const Document &doc, GroupFillet &group);
    static std::shared_ptr<const SolidModel> create(const Document &doc, GroupChamfer &group);
    static std::shared_ptr<const SolidModel> create(const Document &doc, GroupLathe &group);
    static std::shared_ptr<const SolidModel> create(const Document &doc, GroupRevolve &group);
    static std::shared_ptr<const SolidModel> create(const Document &doc, GroupLinearArray &group);
    static std::shared_ptr<const SolidModel> create(const Document &doc, GroupPolarArray &group);
    static std::shared_ptr<const SolidModel> create(const Document &doc, GroupMirrorHV &group);
    static std::shared_ptr<const SolidModel> create(const Document &doc, GroupLoft &group);
    static std::shared_ptr<const SolidModel> create(const Document &doc, GroupSketch &group);
    static std::shared_ptr<const SolidModel> create(const Document &doc, GroupSolidModelOperation &group);
    virtual void export_stl(const std::filesystem::path &path) const = 0;
    virtual void export_step(const std::filesystem::path &path) const = 0;
    virtual void export_projection(const std::filesystem::path &path, const glm::dvec3 &origin,
                                   const glm::dquat &normal) const = 0;

    virtual ~SolidModel();

    enum class IncludeGroup { YES, NO };
    static const SolidModel *get_last_solid_model(const Document &doc, const Group &group,
                                                  IncludeGroup include_group = IncludeGroup::NO);
    static const IGroupSolidModel *get_last_solid_model_group(const Document &doc, const Group &group,
                                                              IncludeGroup include_group = IncludeGroup::NO);
};

} // namespace dune3d
