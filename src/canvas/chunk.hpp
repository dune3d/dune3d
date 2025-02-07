#pragma once
#include <stdint.h>
#include <vector>
#include "icanvas.hpp"
#include <glm/glm.hpp>
#include "vertex_flags.hpp"

namespace dune3d {
class CanvasChunk {
public:
    using VertexFlags = CanvasVertexFlags;

    void clear_flags(VertexFlags flags);
    void clear();
    VertexFlags &get_vertex_flags(const ICanvas::VertexRef &vref);


    class FaceVertex {
    public:
        FaceVertex(float ix, float iy, float iz, float inx, float iny, float inz, uint8_t ir, uint8_t ig, uint8_t ib)
            : x(ix), y(iy), z(iz), nx(inx), ny(iny), nz(inz), r(ir), g(ig), b(ib), _pad(0)
        {
        }
        float x;
        float y;
        float z;
        float nx;
        float ny;
        float nz;

        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t _pad;
    } __attribute__((packed));

    std::vector<FaceVertex> m_face_vertex_buffer;  // vertices of all models, sequentially
    std::vector<unsigned int> m_face_index_buffer; // indexes face_vertex_buffer to form triangles

    size_t m_face_offset = 0;
    size_t m_index_offset = 0;

    class FaceGroup {
    public:
        size_t offset;
        size_t length;
        glm::vec3 origin;
        glm::quat normal;
        ICanvas::FaceColor color;

        VertexFlags flags = VertexFlags::DEFAULT;
    };
    std::vector<FaceGroup> m_face_groups;


    class LineVertex {
    public:
        LineVertex(double ax1, double ay1, double az1, double ax2, double ay2, double az2)
            : x1(ax1), y1(ay1), z1(az1), x2(ax2), y2(ay2), z2(az2)
        {
        }
        LineVertex(glm::vec3 a1, glm::vec3 a2) : x1(a1.x), y1(a1.y), z1(a1.z), x2(a2.x), y2(a2.y), z2(a2.z)
        {
        }
        float x1;
        float y1;
        float z1;
        float x2;
        float y2;
        float z2;

        VertexFlags flags = VertexFlags::DEFAULT;
    };

    std::vector<LineVertex> m_lines;
    std::vector<LineVertex> m_lines_selection_invisible;

    class GlyphVertex {
    public:
        float x0;
        float y0;
        float z0;

        float xs;
        float ys;

        float scale;
        uint32_t bits;

        VertexFlags flags = VertexFlags::DEFAULT;
    };

    std::vector<GlyphVertex> m_glyphs;

    class Glyph3DVertex {
    public:
        float x0;
        float y0;
        float z0;

        float xr;
        float yr;
        float zr;

        float xu;
        float yu;
        float zu;

        uint32_t bits;

        VertexFlags flags = VertexFlags::DEFAULT;
    };

    std::vector<Glyph3DVertex> m_glyphs_3d;

    class IconVertex {
    public:
        float x0;
        float y0;
        float z0;

        float xs;
        float ys;

        float vx;
        float vy;
        float vz;

        uint16_t icon_x;
        uint16_t icon_y;

        VertexFlags flags = VertexFlags::DEFAULT;
    };

    std::vector<IconVertex> m_icons;
    std::vector<IconVertex> m_icons_selection_invisible;
};
} // namespace dune3d
