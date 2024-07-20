#pragma once
#include "util/uuid.hpp"
#include <glm/glm.hpp>
#include <list>
#include "document/group/all_groups_fwd.hpp"
#include "document/entity/entity_visitor.hpp"
#include "document/constraint/constraint_visitor.hpp"
#include "canvas/icanvas.hpp"
#include <optional>
#include <filesystem>

namespace dune3d {

namespace IconTexture {
enum class IconTextureID;
}

class ICanvas;
class IDocumentProvider;
class Document;
class IDocumentView;
class SelectableRef;
enum class ConstraintType;

class Renderer : private EntityVisitor, private ConstraintVisitor {
public:
    Renderer(ICanvas &ca, IDocumentProvider &docprv);
    void render(const Document &doc, const UUID &current_group, const IDocumentView &doc_view,
                const std::filesystem::path &containing_dir, std::optional<SelectableRef> sr);

    bool m_solid_model_edge_select_mode = false;

    void add_constraint_icons(glm::vec3 p, glm::vec3 v, const std::vector<ConstraintType> &constraints);

private:
    void render(const Entity &en);
    void visit(const EntityLine3D &en) override;
    void visit(const EntityLine2D &en) override;
    void visit(const EntityArc2D &en) override;
    void visit(const EntityArc3D &en) override;
    void visit(const EntityCircle2D &en) override;
    void visit(const EntityCircle3D &en) override;
    void visit(const EntityWorkplane &en) override;
    void visit(const EntitySTEP &en) override;
    void visit(const EntityPoint2D &en) override;
    void visit(const EntityDocument &en) override;
    void visit(const EntityBezier2D &en) override;
    void visit(const EntityBezier3D &en) override;
    void visit(const EntityCluster &en) override;
    void visit(const ConstraintPointDistance &constr) override;
    void visit(const ConstraintPointDistanceHV &constr) override;
    void visit(const ConstraintPointsCoincident &constr) override;
    void visit(const ConstraintHV &constr) override;
    void visit(const ConstraintPointOnLine &constr) override;
    void visit(const ConstraintPointOnCircle &constr) override;
    void visit(const ConstraintWorkplaneNormal &constr) override;
    void visit(const ConstraintMidpoint &constr) override;
    void visit(const ConstraintParallel &constr) override;
    void visit(const ConstraintSameOrientation &constr) override;
    void visit(const ConstraintEqualLength &constr) override;
    void visit(const ConstraintEqualRadius &constr) override;
    void visit(const ConstraintDiameterRadius &constr) override;
    void visit(const ConstraintArcArcTangent &constr) override;
    void visit(const ConstraintArcArcTangent &constr, IconTexture::IconTextureID icon);
    void visit(const ConstraintArcLineTangent &constr) override;
    void visit(const ConstraintLinePointsPerpendicular &constr) override;
    void visit(const ConstraintLinesPerpendicular &constr) override;
    void visit(const ConstraintLinesAngle &constr) override;
    void visit(const ConstraintPointInPlane &constr) override;
    void visit(const ConstraintPointLineDistance &constr) override;
    void visit(const ConstraintPointPlaneDistance &constr) override;
    void visit(const ConstraintLockRotation &constr) override;
    void visit(const ConstraintPointInWorkplane &constr) override;
    void visit(const ConstraintSymmetricHV &constr) override;
    void visit(const ConstraintSymmetricLine &constr) override;
    void visit(const ConstraintPointDistanceAligned &constr) override;
    void visit(const ConstraintBezierLineTangent &constr) override;
    void visit(const ConstraintBezierBezierTangentSymmetric &constr) override;

    ICanvas &m_ca;
    IDocumentProvider &m_doc_prv;
    const Document *m_doc = nullptr;
    const IDocumentView *m_doc_view = nullptr;
    const Group *m_current_group = nullptr;
    const Group *m_current_body_group = nullptr;
    std::filesystem::path m_containing_dir;
    bool m_is_current_document = true;
    UUID m_document_uuid;

    bool group_is_visible(const UUID &uu) const;

    struct ConstraintInfo {
        IconTexture::IconTextureID icon;
        glm::vec3 v;
        UUID constraint;
    };
    std::list<std::pair<glm::vec3, std::list<ConstraintInfo>>> m_constraints;

    void add_constraint(const glm::vec3 &pos, IconTexture::IconTextureID icon, const UUID &constraint,
                        const glm::vec3 &v = {NAN, NAN, NAN});
    void draw_constraints();

    void draw_distance_line(const glm::vec3 &from, const glm::vec3 &to, const glm::vec3 &text_p,
                            const std::string &label, const UUID &uu,
                            const glm::vec3 &fallback_normal = {NAN, NAN, NAN});
    void draw_distance_line_with_direction(const glm::vec3 &from, const glm::vec3 &to, const glm::vec3 &dir,
                                           const glm::vec3 &text_p, const std::string &label, const UUID &uu,
                                           const glm::vec3 &fallback_normal = {NAN, NAN, NAN});
    void add_selectables(const SelectableRef &sr, const std::vector<ICanvas::VertexRef> &vrs);
};

} // namespace dune3d
