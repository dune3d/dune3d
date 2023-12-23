#pragma once
#include "base_renderer.hpp"

namespace dune3d {
class FaceRenderer : public BaseRenderer {
public:
    FaceRenderer(class Canvas &c);
    void realize();
    void render();
    void push();

private:
    size_t get_vertex_count() const override;
    void create_vao();

    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_ebo;

    GLuint m_cam_normal_loc;
    GLuint m_flags_loc;
    GLuint m_origin_loc;
    GLuint m_normal_mat_loc;
    GLuint m_override_color_loc;

    GLuint m_clipping_value_loc;
    GLuint m_clipping_op_loc;
};
} // namespace dune3d
