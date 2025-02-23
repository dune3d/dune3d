#pragma once
#include "base_renderer.hpp"
#include <map>
#include "util/uuid.hpp"
#include <memory>

namespace dune3d {

class PictureData;

class PictureRenderer : public BaseRenderer {
public:
    PictureRenderer(class Canvas &c);
    void realize();
    void render();
    void push();

private:
    GLuint m_vao;
    GLuint m_vbo;

    GLuint m_corners_loc;
    GLuint m_tex_loc;
    GLuint m_flags_loc;
    GLuint m_pick_base_loc;

    std::map<UUID, std::pair<std::shared_ptr<const PictureData>, GLuint>> m_textures;
    void cache_picture(std::shared_ptr<const PictureData> d);
    void uncache_picture(const UUID &uu);

    size_t get_vertex_count() const override;

    class PictureWithDepth;
};
} // namespace dune3d
