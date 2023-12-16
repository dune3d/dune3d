#pragma once
#include <glm/glm.hpp>
#include <tuple>
#include "face.hpp"
#include <glm/gtx/quaternion.hpp>

namespace dune3d {

class SelectableRef;

namespace IconTexture {
enum class IconTextureID;
}

class ICanvas {
public:
    enum class VertexType { POINT, LINE, GLYPH, GLYPH_3D, ICON, FACE_GROUP, SELECTION_INVISIBLE };
    struct VertexRef {
        VertexType type;
        size_t index;

        friend auto operator<=>(const VertexRef &, const VertexRef &) = default;
        friend bool operator==(const VertexRef &, const VertexRef &) = default;
    };

    virtual void clear() = 0;
    virtual VertexRef draw_point(glm::vec3 p) = 0;
    virtual VertexRef draw_line(glm::vec3 from, glm::vec3 to) = 0;
    virtual VertexRef draw_screen_line(glm::vec3 origin, glm::vec3 direction) = 0;
    virtual std::vector<VertexRef> draw_bitmap_text(const glm::vec3 p, float size, const std::string &rtext) = 0;
    virtual std::vector<VertexRef> draw_bitmap_text_3d(const glm::vec3 p, const glm::quat &norm, float size,
                                                       const std::string &rtext) = 0;

    // virtual void add_faces(const face::Faces &faces) = 0;
    enum class FaceColor { AS_IS, SOLID_MODEL, OTHER_BODY_SOLID_MODEL };
    virtual VertexRef add_face_group(const face::Faces &faces, glm::vec3 origin, glm::quat normal,
                                     FaceColor face_color) = 0;
    virtual VertexRef draw_icon(IconTexture::IconTextureID id, glm::vec3 origin, glm::vec2 shift) = 0;
    virtual void set_vertex_inactive(bool inactive) = 0;
    virtual void set_vertex_constraint(bool c) = 0;
    virtual void set_vertex_construction(bool c) = 0;

    virtual void add_selectable(const VertexRef &vref, const SelectableRef &sref) = 0;
    virtual void set_selection_invisible(bool selection_invisible) = 0;

    virtual void update_bbox() = 0;
};
} // namespace dune3d
