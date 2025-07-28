#pragma once
#include <string>
#include <map>
#include "document_view.hpp"
#include "iworkspace_view.hpp"
#include "nlohmann/json_fwd.hpp"
#include "canvas/projection.hpp"
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <filesystem>

namespace dune3d {

using json = nlohmann::json;

class WorkspaceView : public IWorkspaceView {
public:
    explicit WorkspaceView(const json &j);
    WorkspaceView();
    static std::map<UUID, WorkspaceView> load_from_json(const json &j);
    static json serialize(const std::map<UUID, WorkspaceView> &views, const UUID &doc_uu);

    std::string m_name;

    std::map<UUID, DocumentView> m_documents;
    UUID m_current_document;

    glm::vec3 m_center = glm::vec3(0, 0, 0);
    float m_cam_distance = 100;
    CanvasProjection m_projection = CanvasProjection::ORTHO;
    glm::quat m_cam_quat = glm::quat_identity<float, glm::defaultp>();
    float m_curvature_comb_scale = 0;
    bool m_show_construction_entities_from_previous_groups = false;
    bool construction_entities_from_previous_groups_are_visible() const override
    {
        return m_show_construction_entities_from_previous_groups;
    }

    bool m_hide_irrelevant_workplanes = false;
    bool hide_irrelevant_workplanes() const override
    {
        return m_hide_irrelevant_workplanes;
    }
    float get_curvature_comb_scale() const override
    {
        return m_curvature_comb_scale;
    }


    bool document_is_visible(const UUID &uu_doc) const;
    json serialize(const UUID &uu_doc) const;
};
} // namespace dune3d
