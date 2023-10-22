#include "base_renderer.hpp"
#include "gl_util.hpp"
#include "canvas.hpp"
#include "color_palette.hpp"
#include <glm/gtc/type_ptr.hpp>

namespace dune3d {

BaseRenderer::BaseRenderer(Canvas &c, ICanvas::VertexType vertex_type) : m_ca(c), m_vertex_type(vertex_type)
{
}

void BaseRenderer::realize_base()
{
    glGenBuffers(1, &m_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);

    unsigned int block_index = glGetUniformBlockIndex(m_program, "color_setup");
    GLuint binding_point_index = static_cast<GLuint>(m_vertex_type);
    glBindBufferBase(GL_UNIFORM_BUFFER, binding_point_index, m_ubo);
    glUniformBlockBinding(m_program, block_index, binding_point_index);

    GET_LOC(this, view);
    GET_LOC(this, proj);
    GET_LOC(this, pick_base);
}


struct UBOBuffer {
    std::array<std::array<float, 4>, 32> colors;
    void set_colors(const Appearance &appearance, Canvas::SelectionMode selection_mode, Canvas::VertexType type)
    {
        using F = Canvas::VertexFlags;
        using T = Canvas::VertexType;

        switch (type) {
        case T::LINE:
        case T::ICON:
        case T::GLYPH:
            set_color(F::DEFAULT, appearance, ColorP::ENTITY);
            set_color(F::INACTIVE, appearance, ColorP::INACTIVE_ENTITY);
            set_color(F::CONSTRUCTION, appearance, ColorP::CONSTRUCTION_ENTITY);
            set_color(F::CONSTRAINT, appearance, ColorP::CONSTRAINT);
            break;

        case T::POINT:
            set_color(F::DEFAULT, appearance, ColorP::POINT);
            set_color(F::INACTIVE, appearance, ColorP::INACTIVE_POINT);
            set_color(F::CONSTRUCTION, appearance, ColorP::CONSTRUCTION_POINT);
            set_color(F::CONSTRAINT, appearance, ColorP::CONSTRAINT);
            break;

        case T::SELECTION_INVISIBLE:
        case T::FACE_GROUP:;
        }

        for (const auto f : {F::CONSTRAINT, F::CONSTRUCTION, F::INACTIVE, F::DEFAULT}) {
            if (selection_mode == Canvas::SelectionMode::HOVER) {
                set_color(f | F::SELECTED, appearance, ColorP::SELECTED);
                set_color(f | F::HOVER, appearance, ColorP::SELECTED);
                set_color(f | F::HOVER | F::SELECTED, appearance, ColorP::SELECTED);
            }
            else {
                set_color(f | F::SELECTED, appearance, ColorP::SELECTED);
                set_color(f | F::HOVER, appearance, ColorP::HOVER);
                set_color(f | F::HOVER | F::SELECTED, appearance, ColorP::SELECTED_HOVER);
            }
        }
    }

private:
    void set_color(Canvas::VertexFlags flags, const Appearance &appearance, ColorP colorp)
    {
        using F = Canvas::VertexFlags;
        const auto &color = appearance.get_color(colorp);
        static const auto color_mask = F::SELECTED | F::INACTIVE | F::HOVER | F::CONSTRAINT | F::CONSTRUCTION;
        auto &e = colors.at(static_cast<size_t>(flags & color_mask));
        std::get<0>(e) = color.r;
        std::get<1>(e) = color.g;
        std::get<2>(e) = color.b;
    }
};

void BaseRenderer::load_uniforms()
{
    glUniformMatrix4fv(m_view_loc, 1, GL_FALSE, glm::value_ptr(m_ca.m_viewmat));
    glUniformMatrix4fv(m_proj_loc, 1, GL_FALSE, glm::value_ptr(m_ca.m_projmat));
    glUniform1ui(m_pick_base_loc, m_ca.m_pick_base);
    m_ca.m_vertex_type_picks[m_vertex_type] = {.offset = m_ca.m_pick_base, .count = get_vertex_count()};
    m_ca.m_pick_base += get_vertex_count();

    {
        UBOBuffer buf;
        buf.set_colors(m_ca.m_appearance, m_ca.m_selection_mode, m_vertex_type);
        glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(buf), &buf, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
}

} // namespace dune3d
