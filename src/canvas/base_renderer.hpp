#pragma once
#include <epoxy/gl.h>
#include "icanvas.hpp"

namespace dune3d {
class BaseRenderer {
public:
    BaseRenderer(class Canvas &c, ICanvas::VertexType vertex_type);
    void set_peeled_picks(const std::vector<unsigned int> &peeled_picks);

protected:
    void realize_base();

    Canvas &m_ca;
    const ICanvas::VertexType m_vertex_type;

    void load_uniforms();
    std::vector<unsigned int> m_peeled_picks;

    GLuint m_program;
    GLuint m_ubo;

    GLuint m_view_loc;
    GLuint m_proj_loc;
    GLuint m_pick_base_loc;

    unsigned int m_type_pick_base = 1;
};
} // namespace dune3d
