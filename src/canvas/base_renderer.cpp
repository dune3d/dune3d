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
    // keep in sync with ubo.glsl
    std::array<std::array<float, 4>, 64> colors;
    struct I {
        unsigned int value;
        uint8_t pad[16 - sizeof(unsigned int)];
    };
    static_assert(sizeof(I) == 16);
    std::array<I, 8> peeled_picks;
    void set_colors(const Appearance &appearance, SelectionMode selection_mode, Canvas::VertexType type)
    {
        using F = Canvas::VertexFlags;
        using T = Canvas::VertexType;

        switch (type) {
        case T::ICON:
            set_color(F::DEFAULT, appearance, ColorP::POINT);
            set_color(F::INACTIVE, appearance, ColorP::INACTIVE_POINT);
            set_color(F::CONSTRUCTION, appearance, ColorP::CONSTRUCTION_POINT);
            set_color(F::CONSTRUCTION | F::INACTIVE, appearance, ColorP::CONSTRUCTION_POINT);
            set_color(F::CONSTRAINT, appearance, ColorP::CONSTRAINT);

            break;
        case T::LINE:
        case T::GLYPH:
        case T::GLYPH_3D:
            set_color(F::DEFAULT, appearance, ColorP::ENTITY);
            set_color(F::INACTIVE, appearance, ColorP::INACTIVE_ENTITY);
            set_color(F::CONSTRUCTION, appearance, ColorP::CONSTRUCTION_ENTITY);
            set_color(F::CONSTRUCTION | F::INACTIVE, appearance, ColorP::CONSTRUCTION_ENTITY);
            set_color(F::CONSTRAINT, appearance, ColorP::CONSTRAINT);
            break;

        case T::SELECTION_INVISIBLE:
        case T::FACE_GROUP:;
        }

        for (const auto f : {F::CONSTRAINT, F::CONSTRUCTION, F::INACTIVE, F::DEFAULT, F::CONSTRUCTION | F::INACTIVE}) {
            if (selection_mode == SelectionMode::HOVER) {
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
        for (uint32_t i = 0; i <= static_cast<uint32_t>(F::COLOR_MASK); i++) {
            set_color(static_cast<F>(i) | F::HIGHLIGHT, appearance, ColorP::HIGHLIGHT);
        }
    }

    void set_peeled_picks(const std::vector<unsigned int> &peeled_picks_vec)
    {
        std::ranges::fill(peeled_picks, I{0});
        for (size_t i = 0; i < std::min(peeled_picks_vec.size(), peeled_picks.size()); i++) {
            peeled_picks.at(i).value = peeled_picks_vec.at(i);
        }
    }

private:
    void set_color(Canvas::VertexFlags flags, const Appearance &appearance, ColorP colorp)
    {
        using F = Canvas::VertexFlags;
        const auto &color = appearance.get_color(colorp);
        auto &e = colors.at(static_cast<size_t>(flags & F::COLOR_MASK));
        std::get<0>(e) = color.r;
        std::get<1>(e) = color.g;
        std::get<2>(e) = color.b;
    }
};

void BaseRenderer::load_uniforms()
{
    static_assert(Canvas::s_peel_max == std::tuple_size_v<decltype(UBOBuffer::peeled_picks)>);

    glUniformMatrix4fv(m_view_loc, 1, GL_FALSE, glm::value_ptr(m_ca.m_viewmat));
    glUniformMatrix4fv(m_proj_loc, 1, GL_FALSE, glm::value_ptr(m_ca.m_projmat));
    glUniform1ui(m_pick_base_loc, m_ca.m_pick_base);
    m_ca.m_vertex_type_picks[m_vertex_type] = {.offset = m_ca.m_pick_base, .count = get_vertex_count()};
    m_ca.m_pick_base += get_vertex_count();

    {
        UBOBuffer buf;
        buf.set_colors(m_ca.m_appearance, m_ca.m_selection_mode, m_vertex_type);
        buf.set_peeled_picks(m_peeled_picks);
        glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(buf), &buf, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
}

void BaseRenderer::set_peeled_picks(const std::vector<unsigned int> &peeled_picks)
{
    m_peeled_picks = peeled_picks;
}


} // namespace dune3d
