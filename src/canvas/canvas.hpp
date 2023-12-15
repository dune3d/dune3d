#pragma once
#include <gtkmm.h>
#include "background_renderer.hpp"
#include "face_renderer.hpp"
#include "point_renderer.hpp"
#include "line_renderer.hpp"
#include "glyph_renderer.hpp"
#include "icon_renderer.hpp"
#include "box_selection.hpp"
#include "icanvas.hpp"
#include "color.hpp"
#include "bitmask_operators.hpp"
#include "selectable_ref.hpp"
#include "face.hpp"
#include "appearance.hpp"
#include "util/msd_animator.hpp"
#include <glm/glm.hpp>

namespace dune3d {
class Canvas : public Gtk::GLArea, public ICanvas {
public:
    friend BackgroundRenderer;
    friend FaceRenderer;
    friend PointRenderer;
    friend LineRenderer;
    friend GlyphRenderer;
    friend IconRenderer;
    friend BaseRenderer;
    friend struct UBOBuffer;
    friend BoxSelection;
    Canvas();

    void request_push();
    void queue_pick();

    void clear() override;
    VertexRef draw_point(glm::vec3 p) override;
    VertexRef draw_line(glm::vec3 from, glm::vec3 to) override;
    VertexRef draw_screen_line(glm::vec3 origin, glm::vec3 direction) override;
    std::vector<VertexRef> draw_bitmap_text(const glm::vec3 p, float size, const std::string &rtext,
                                            int angle) override;
    void add_selectable(const VertexRef &vref, const SelectableRef &sref) override;
    void set_vertex_inactive(bool inactive) override
    {
        m_vertex_inactive = inactive;
    }
    void set_vertex_constraint(bool c) override
    {
        m_vertex_constraint = c;
    }
    void set_vertex_construction(bool c) override
    {
        m_vertex_construction = c;
    }

    VertexRef add_face_group(const face::Faces &faces, glm::vec3 origin, glm::quat normal,
                             FaceColor face_color) override;

    VertexRef draw_icon(IconTexture::IconTextureID id, glm::vec3 origin, glm::vec2 shift) override;

    glm::dvec3 get_cursor_pos() const;
    glm::dvec3 get_cursor_pos_for_plane(glm::dvec3 origin, glm::dvec3 normal) const;
    glm::vec3 get_cam_normal() const;
    glm::dvec2 get_cursor_pos_win() const;

    enum class SelectionMode { HOVER, NORMAL, HOVER_ONLY, DRAG, NONE };
    void set_selection_mode(SelectionMode mode);
    SelectionMode get_selection_mode() const
    {
        return m_selection_mode;
    }

    void set_selection(const std::set<SelectableRef> &sel, bool emit);
    std::set<SelectableRef> get_selection() const;
    auto get_hover_selection() const
    {
        return m_hover_selection;
    }

    void set_hover_selection(const std::optional<SelectableRef> &sr);

    void set_selection_invisible(bool selection_invisible) override
    {
        m_selection_invisible = selection_invisible;
    }

    void set_cam_quat(const glm::quat &q);

    const glm::quat &get_cam_quat() const
    {
        return m_cam_quat;
    }

    float get_cam_distance() const
    {
        return m_cam_distance;
    }
    void set_cam_distance(float dist);

    glm::vec3 get_center() const
    {
        return m_center;
    }
    void set_center(glm::vec3 center);

    void animate_to_cam_quat(const glm::quat &quat);
    void animate_to_center_abs(const glm::vec3 &center);

    enum class Projection { PERSP, ORTHO };
    void set_projection(Projection proj);
    Projection get_projection() const
    {
        return m_projection;
    }

    void set_enable_animations(bool e)
    {
        m_enable_animations = e;
    }
    bool get_enable_animations()
    {
        return m_enable_animations;
    }

    typedef sigc::signal<void()> type_signal_view_changed;
    type_signal_view_changed signal_view_changed()
    {
        return m_signal_view_changed;
    }

    type_signal_view_changed signal_cursor_moved()
    {
        return m_signal_cursor_moved;
    }

    type_signal_view_changed signal_selection_changed()
    {
        return m_signal_selection_changed;
    }

    type_signal_view_changed signal_hover_selection_changed()
    {
        return m_signal_hover_selection_changed;
    }

    type_signal_view_changed signal_selection_mode_changed()
    {
        return m_signal_selection_mode_changed;
    }

    void set_appearance(const Appearance &appearance);


    void end_pan();

    void set_highlight(const std::set<SelectableRef> &sr);

    void inhibit_drag_selection()
    {
        m_inhibit_drag_selection = true;
    }

    void setup_controllers();

    void update_bbox() override;

private:
    BackgroundRenderer m_background_renderer;
    FaceRenderer m_face_renderer;
    PointRenderer m_point_renderer;
    LineRenderer m_line_renderer;
    GlyphRenderer m_glyph_renderer;
    IconRenderer m_icon_renderer;
    BoxSelection m_box_selection;
    unsigned int m_pick_base = 1;

    Appearance m_appearance;

    void on_realize() override;
    bool on_render(const Glib::RefPtr<Gdk::GLContext> &context) override;
    void on_resize(int width, int height) override;
    void resize_buffers();

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

    glm::mat4 m_viewmat;
    glm::mat4 m_projmat;
    glm::mat4 m_projmat_viewmat_inv;
    glm::mat3 m_screenmat;
    glm::vec3 m_cam_normal;

    glm::dvec2 m_cursor_pos;

    enum class PickState { QUEUED, CURRENT, INVALID };
    PickState m_pick_state = PickState::INVALID;

    std::vector<uint16_t> m_pick_buf;
    uint16_t read_pick_buf(int x, int y) const;

    GLuint m_renderbuffer;
    GLuint m_fbo;
    GLuint m_depthrenderbuffer;
    GLuint m_pickrenderbuffer;

    GLuint m_fbo_downsampled;
    GLuint m_pickrenderbuffer_downsampled;

    enum PushFlags {
        PF_NONE = 0,
        PF_FACES = (1 << 0),
        PF_POINTS = (1 << 1),
        PF_LINES = (1 << 2),
        PF_GLYPHS = (1 << 3),
        PF_ICONS = (1 << 4),
        PF_ALL = 0xff,
    };
    PushFlags m_push_flags = PF_ALL;

    int m_dev_width = 100;
    int m_dev_height = 100;
    int m_width = 100;
    int m_height = 100;
    bool m_needs_resize = false;

    glm::quat m_cam_quat;
    float m_cam_distance = 10;
    float m_cam_fov = 45;
    glm::vec3 m_center = {0, 0, 0};
    Projection m_projection = Projection::ORTHO;

    std::pair<glm::vec3, glm::vec3> m_bbox;

    MSDAnimator m_quat_x_animator;
    MSDAnimator m_quat_y_animator;
    MSDAnimator m_quat_z_animator;
    MSDAnimator m_quat_w_animator;
    MSDAnimator m_zoom_animator;
    MSDAnimator m_cx_animator;
    MSDAnimator m_cy_animator;
    MSDAnimator m_cz_animator;

    std::vector<MSDAnimator *> m_animators;
    int animate_step(GdkFrameClock *frame_clock);
    static int anim_tick_cb(GtkWidget *cwidget, GdkFrameClock *frame_clock, gpointer user_data);
    void start_anim();
    bool m_enable_animations = true;

    void scroll_zoom(double dx, double dy, Gtk::EventController &ctrl);
    void scroll_move(double dx, double dy, Gtk::EventController &ctrl);
    void scroll_rotate(double dx, double dy, Gtk::EventController &ctrl);

    Glib::RefPtr<Gtk::GestureDrag> m_gesture_drag;
    glm::vec3 m_gesture_drag_center_orig;
    void drag_gesture_begin_cb(Gdk::EventSequence *seq);
    void drag_gesture_update_cb(Gdk::EventSequence *seq);

    Glib::RefPtr<Gtk::GestureZoom> m_gesture_zoom;
    float m_gesture_zoom_cam_dist_orig = 1;
    void zoom_gesture_begin_cb(Gdk::EventSequence *seq);
    void zoom_gesture_update_cb(Gdk::EventSequence *seq);

    Glib::RefPtr<Gtk::GestureRotate> m_gesture_rotate;
    glm::quat m_gesture_rotate_cam_quat_orig;
    glm::vec2 m_gesture_rotate_pos_orig;
    void rotate_gesture_begin_cb(Gdk::EventSequence *seq);
    void rotate_gesture_update_cb(Gdk::EventSequence *seq);


    enum class PanMode { NONE, MOVE, ROTATE, TILT };
    PanMode m_pan_mode = PanMode::NONE;

    void handle_click_release();

    glm::vec2 m_pointer_pos_orig;
    glm::quat m_cam_quat_orig;

    glm::vec3 m_center_orig;
    glm::vec3 get_center_shift(const glm::vec2 &shift) const;

    enum class VertexFlags : uint32_t {
        DEFAULT = 0,
        SELECTED = (1 << 0),
        HOVER = (1 << 1),
        INACTIVE = (1 << 2),
        CONSTRAINT = (1 << 3),
        CONSTRUCTION = (1 << 4),
        HIGHLIGHT = (1 << 5),
        SCREEN = (1 << 6),
        COLOR_MASK = SELECTED | HOVER | INACTIVE | CONSTRAINT | CONSTRUCTION | HIGHLIGHT,
    };
    bool m_vertex_inactive = false;
    bool m_vertex_constraint = false;
    bool m_vertex_construction = false;

    class PointVertex {
    public:
        PointVertex(double ax, double ay, double az) : x(ax), y(ay), z(az)
        {
        }
        PointVertex(glm::vec3 a) : x(a.x), y(a.y), z(a.z)
        {
        }
        float x;
        float y;
        float z;


        VertexFlags flags = VertexFlags::DEFAULT;
    };

    std::vector<PointVertex> m_points;
    std::vector<PointVertex> m_points_selection_invisible;
    size_t m_n_points = 0;
    size_t m_n_points_selection_invisible = 0;

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
    size_t m_n_lines = 0;
    size_t m_n_lines_selection_invisible = 0;

    bool m_selection_invisible = false;

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
    size_t m_n_glyphs = 0;

    class IconVertex {
    public:
        float x0;
        float y0;
        float z0;

        float xs;
        float ys;

        uint16_t icon_x;
        uint16_t icon_y;

        VertexFlags flags = VertexFlags::DEFAULT;
    };

    std::vector<IconVertex> m_icons;
    size_t m_n_icons = 0;

    void add_faces(const face::Faces &faces);
    class FaceGroup {
    public:
        size_t offset;
        size_t length;
        glm::vec3 origin;
        glm::quat normal;
        FaceColor color;

        VertexFlags flags = VertexFlags::DEFAULT;
    };

    std::vector<FaceGroup> m_face_groups;

    std::map<VertexRef, SelectableRef> m_vertex_to_selectable_map;
    std::map<SelectableRef, std::vector<VertexRef>> m_selectable_to_vertex_map;


    VertexFlags &get_vertex_flags(const VertexRef &vref);

    struct PickInfo {
        size_t offset;
        size_t count;
    };

    std::map<VertexType, PickInfo> m_vertex_type_picks;
    VertexRef get_vertex_ref_for_pick(unsigned int pick) const;
    std::optional<SelectableRef> get_selectable_ref_for_vertex_ref(const VertexRef &vref) const;
    std::optional<SelectableRef> get_selectable_ref_for_pick(unsigned int pick) const;

    SelectionMode m_selection_mode = SelectionMode::HOVER;
    std::optional<SelectableRef> m_hover_selection;

    double m_last_x = 0, m_last_y = 0;
    void update_hover_selection();

    type_signal_view_changed m_signal_view_changed;
    type_signal_view_changed m_signal_cursor_moved;
    type_signal_view_changed m_signal_selection_changed;
    type_signal_view_changed m_signal_hover_selection_changed;
    type_signal_view_changed m_signal_selection_mode_changed;

    void apply_flags(VertexFlags &flags);

    void set_flag_for_selectables(const std::set<SelectableRef> &sr, VertexFlags flag);

    glm::vec2 m_drag_selection_start;
    SelectionMode m_last_selection_mode = SelectionMode::NONE;
    bool m_dragging = false;
    void update_drag_selection(glm::vec2 pos);
    bool m_inhibit_drag_selection = false;

    int m_scale_factor = 1;
};

} // namespace dune3d


template <> struct enable_bitmask_operators<dune3d::Canvas::VertexFlags> {
    static constexpr bool enable = true;
};
