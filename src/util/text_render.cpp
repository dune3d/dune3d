#include "text_render.hpp"
#include "document/entity/entity_text.hpp"
#include "document/entity/entity_line2d.hpp"
#include "document/entity/entity_bezier2d.hpp"
#include "document/document.hpp"
#include "document/group/group_reference.hpp"
#include "util/cluster_content.hpp"
#include <freetype/freetype.h>
#include <freetype/ftoutln.h>
#include <hb.h>
#include <hb-ft.h>

namespace dune3d {

static constexpr double scale_factor = 1e3;

class EntityAdder {
public:
    EntityAdder(ClusterContent &cluster, const UUID &group, const UUID &wrkpl)
        : m_cluster(cluster), m_group(group), m_wrkpl(wrkpl)
    {
    }

    template <typename T> T &add_entity()
    {
        auto uu = UUID::random();
        auto en = std::make_unique<T>(uu);
        en->m_wrkpl = m_wrkpl;
        en->m_group = m_group;
        auto p = en.get();
        m_cluster.m_entities.emplace(uu, std::move(en));
        return *p;
    }

    static int move_to(const FT_Vector *p, void *pself)
    {
        auto self = static_cast<EntityAdder *>(pself);
        self->m_p = {p->x, p->y};
        return 0;
    }

    static int line_to(const FT_Vector *p, void *pself)
    {
        auto self = static_cast<EntityAdder *>(pself);
        glm::dvec2 to{p->x, p->y};

        auto &en = self->add_entity<EntityLine2D>();
        en.m_p1 = self->transform(self->m_p);
        en.m_p2 = self->transform(to);

        self->m_p = to;
        return 0;
    }

    static int conic_to(const FT_Vector *control, const FT_Vector *to, void *pself)
    {
        auto self = static_cast<EntityAdder *>(pself);
        glm::dvec2 gto{to->x, to->y};
        const glm::dvec2 c1{control->x, control->y};

        auto &en = self->add_entity<EntityBezier2D>();
        en.m_p1 = self->transform(self->m_p);
        // https://fontforge.org/docs/techref/bezier.html#converting-truetype-to-postscript
        en.m_c1 = self->transform(self->m_p + (2. / 3) * (c1 - self->m_p));
        en.m_c2 = self->transform(gto + (2. / 3) * (c1 - gto));
        en.m_p2 = self->transform(gto);


        self->m_p = gto;

        return 0;
    }
    static int cubic_to(const FT_Vector *control1, const FT_Vector *control2, const FT_Vector *to, void *pself)
    {

        auto self = static_cast<EntityAdder *>(pself);
        const glm::dvec2 gto{to->x, to->y};
        const glm::dvec2 c1{control1->x, control1->y};
        const glm::dvec2 c2{control2->x, control2->y};

        auto &en = self->add_entity<EntityBezier2D>();
        en.m_p1 = self->transform(self->m_p);
        en.m_c1 = self->transform(c1);
        en.m_c2 = self->transform(c2);
        en.m_p2 = self->transform(gto);


        self->m_p = gto;

        return 0;
    }

    glm::dvec2 m_shift;

private:
    ClusterContent &m_cluster;
    UUID m_group;
    UUID m_wrkpl;

    glm::dvec2 m_p;

    glm::dvec2 transform(const glm::dvec2 &p) const
    {
        return (p + m_shift) / scale_factor;
    }
};

void render_text(EntityText &text, Glib::RefPtr<Pango::Context> ctx, const Document &doc)
{
    hb_buffer_t *buf;
    buf = hb_buffer_create();
    hb_buffer_add_utf8(buf, text.m_text.c_str(), text.m_text.size(), 0, -1);
    hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
    hb_buffer_set_script(buf, HB_SCRIPT_LATIN);
    hb_buffer_set_language(buf, hb_language_from_string("en", -1));


    Pango::FontDescription descr{text.m_font};
    auto pfont = ctx->load_font(descr);


    auto hb_font = pango_font_get_hb_font(pfont->gobj());

    auto hb_font_copy = hb_font_create_sub_font(hb_font);

    hb_ft_font_set_funcs(hb_font_copy);
    auto face2 = hb_ft_font_lock_face(hb_font_copy);

    std::vector<hb_feature_t> features;
    {
        std::string temp;
        std::istringstream ss(text.m_font_features);
        while (std::getline(ss, temp, ',')) {
            hb_feature_from_string(temp.c_str(), temp.size(), &features.emplace_back());
        }
    }

    hb_shape(hb_font, buf, features.data(), features.size());
    unsigned int glyph_count;
    hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
    hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);

    FT_Outline_Funcs funcs;
    funcs.move_to = &EntityAdder::move_to;
    funcs.line_to = &EntityAdder::line_to;
    funcs.conic_to = &EntityAdder::conic_to;
    funcs.cubic_to = &EntityAdder::cubic_to;
    funcs.shift = 0;
    funcs.delta = 0;

    auto content = ClusterContent::create();

    EntityAdder adder{*content, text.m_group, doc.get_reference_group().get_workplane_xy_uuid()};

    hb_position_t cursor_x = 0;
    hb_position_t cursor_y = 0;

    int top = 0;
    int bottom = 0;
    int right = 0;

    for (unsigned int i = 0; i < glyph_count; i++) {
        hb_codepoint_t glyphid = glyph_info[i].codepoint;
        hb_position_t x_offset = glyph_pos[i].x_offset;
        hb_position_t y_offset = glyph_pos[i].y_offset;
        hb_position_t x_advance = glyph_pos[i].x_advance;
        hb_position_t y_advance = glyph_pos[i].y_advance;

        auto rc = FT_Load_Glyph(face2,
                                glyphid, // the glyph_index in the font file
                                FT_LOAD_DEFAULT);
        if (rc != 0)
            continue;

        adder.m_shift = {cursor_x + x_offset, cursor_y + y_offset};
        hb_glyph_extents_t extents;
        if (hb_font_get_glyph_extents(hb_font, glyphid, &extents)) {
            top = std::max(top, extents.y_bearing);
            bottom = std::min(bottom, extents.y_bearing + extents.height);
            right = std::max(right, cursor_x + x_offset + extents.x_bearing + extents.width);
        }

        FT_Outline_Decompose(&face2->glyph->outline, &funcs, &adder);

        cursor_x += x_advance;
        cursor_y += y_advance;
    }

    text.clear_anchors();
    hb_font_extents_t extents;

    if (!hb_font_get_h_extents(hb_font, &extents)) {
        extents.ascender = top;
        extents.descender = bottom;
    }
    for (const auto ax : {EntityText::AnchorX::LEFT, EntityText::AnchorX::RIGHT}) {
        const auto x = (ax == EntityText::AnchorX::RIGHT) ? right : 0;
        for (const auto ay : {EntityText::AnchorY::BOTTOM, EntityText::AnchorY::DESCEND, EntityText::AnchorY::BASE,
                              EntityText::AnchorY::ASCEND, EntityText::AnchorY::TOP}) {
            if (ax == EntityText::AnchorX::LEFT && ay == EntityText::AnchorY::BASE)
                continue;
            const auto id = text.get_anchor_index(ax, ay);
            int y = 0;
            switch (ay) {
            case EntityText::AnchorY::BOTTOM:
                y = bottom;
                break;

            case EntityText::AnchorY::DESCEND:
                y = extents.descender;
                break;

            case EntityText::AnchorY::ASCEND:
                y = extents.ascender;
                break;

            case EntityText::AnchorY::TOP:
                y = top;
                break;

            case EntityText::AnchorY::BASE:
                y = 0;
                break;
            }
            text.add_anchor(id, {x / scale_factor, y / scale_factor});
        }
    }

    hb_buffer_destroy(buf);
    hb_ft_font_unlock_face(hb_font);


    text.m_content = content;
}

} // namespace dune3d
