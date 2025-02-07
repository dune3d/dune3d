#include "chunk.hpp"
#include <stdexcept>

namespace dune3d {

void CanvasChunk::clear_flags(VertexFlags mask)
{
    for (auto &x : m_lines) {
        x.flags &= ~mask;
    }
    for (auto &x : m_glyphs) {
        x.flags &= ~mask;
    }
    for (auto &x : m_glyphs_3d) {
        x.flags &= ~mask;
    }
    for (auto &x : m_face_groups) {
        x.flags &= ~mask;
    }
    for (auto &x : m_icons) {
        x.flags &= ~mask;
    }
}

CanvasChunk::VertexFlags &CanvasChunk::get_vertex_flags(const ICanvas::VertexRef &vref)
{
    using VertexType = ICanvas::VertexType;
    switch (vref.type) {
    case VertexType::LINE:
        return m_lines.at(vref.index).flags;

    case VertexType::GLYPH:
        return m_glyphs.at(vref.index).flags;

    case VertexType::GLYPH_3D:
        return m_glyphs_3d.at(vref.index).flags;

    case VertexType::FACE_GROUP:
        return m_face_groups.at(vref.index).flags;

    case VertexType::ICON:
        return m_icons.at(vref.index).flags;

    default:
        throw std::runtime_error("unknown vertex type");
    }
}

void CanvasChunk::clear()
{
    m_face_index_buffer.clear();
    m_face_vertex_buffer.clear();
    m_face_groups.clear();
    m_lines.clear();
    m_lines_selection_invisible.clear();
    m_glyphs.clear();
    m_glyphs_3d.clear();
    m_icons.clear();
    m_icons_selection_invisible.clear();
}

} // namespace dune3d
