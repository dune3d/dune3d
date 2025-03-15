#include "picture_renderer.hpp"
#include "canvas.hpp"
#include "gl_util.hpp"
#include "util/picture_data.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <numeric>

namespace dune3d {

static GLuint create_vao(GLuint program)
{
    GLuint vao;

    /* we need to create a VAO to store the other buffers */
    glGenVertexArrays(1, &vao);

    return vao;
}

PictureRenderer::PictureRenderer(Canvas &ca) : BaseRenderer(ca, Canvas::VertexType::PICTURE)
{
}

void PictureRenderer::realize()
{
    m_program = gl_create_program_from_resource("/org/dune3d/dune3d/canvas/shaders/picture-vertex.glsl",
                                                "/org/dune3d/dune3d/canvas/shaders/picture-fragment.glsl", nullptr);
    m_vao = create_vao(m_program);

    realize_base();

    GET_LOC(this, corners);
    GET_LOC(this, tex);
    GET_LOC(this, pick_base);
    GET_LOC(this, flags);
}

class PictureRenderer::PictureWithDepth {
public:
    const CanvasChunk::Picture &pic;
    unsigned int chunk;
    size_t index;
    double depth;
};

void PictureRenderer::render()
{
    glUseProgram(m_program);
    glBindVertexArray(m_vao);
    GL_CHECK_ERROR
    load_uniforms();
    GL_CHECK_ERROR
    glActiveTexture(GL_TEXTURE2);
    glUniform1i(m_tex_loc, 2);
    GL_CHECK_ERROR
    glDepthMask(false);

    std::list<PictureWithDepth> pictures_sorted;
    {
        const auto chunk_ids = m_ca.get_chunk_ids();
        for (const auto chunk_id : chunk_ids) {
            const auto &chunk = m_ca.m_chunks.at(chunk_id);
            size_t pic_idx = 0;
            for (const auto &it : chunk.m_pictures) {
                const auto org =
                        std::accumulate(it.corners.begin(), it.corners.end(), glm::vec3{0.}) / (float)it.corners.size();
                auto org_t = (m_ca.m_projmat * m_ca.m_viewmat) * glm::vec4(org, 1.f);
                org_t /= org_t.w;
                pictures_sorted.emplace_back(it, chunk_id, pic_idx++, org_t.z);
            }
        }
    }
    pictures_sorted.sort([](const auto &a, const auto &b) { return a.depth > b.depth; });

    for (const auto &[pic, chunk_id, idx, depth] : pictures_sorted) {
        const auto &tex = m_textures.at(pic.data->m_uuid);
        glBindTexture(GL_TEXTURE_2D, tex.second);
        glUniform1ui(m_pick_base_loc,
                     m_ca.m_vertex_type_picks.at({Canvas::VertexType::PICTURE, chunk_id}).offset + idx);
        glUniform1ui(m_flags_loc, static_cast<uint32_t>(pic.flags));
        glUniform3fv(m_corners_loc, pic.corners.size(), glm::value_ptr(pic.corners.front()));

        if (pic.selection_invisible)
            glColorMaski(1, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }
    glDepthMask(true);
    GL_CHECK_ERROR

    glBindVertexArray(0);
    glUseProgram(0);
}

void PictureRenderer::cache_picture(std::shared_ptr<const PictureData> d)
{
    if (m_textures.count(d->m_uuid))
        return;
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, d->m_width, d->m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, d->m_data.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    m_textures.emplace(std::piecewise_construct, std::forward_as_tuple(d->m_uuid), std::forward_as_tuple(d, tex));
}

void PictureRenderer::uncache_picture(const UUID &uu)
{
    glDeleteTextures(1, &m_textures.at(uu).second);
    m_textures.erase(uu);
}

void PictureRenderer::push()
{
    std::set<UUID> pics_to_evict, pics_needed;
    for (auto &chunk : m_ca.m_chunks) {
        for (const auto &it : chunk.m_pictures) {
            if (!m_textures.count(it.data->m_uuid))
                cache_picture(it.data);
            pics_needed.insert(it.data->m_uuid);
        }
    }
    for (const auto &[uu, it] : m_textures) {
        if (!pics_needed.count(uu))
            pics_to_evict.insert(uu);
    }
    for (const auto &it : pics_to_evict) {
        uncache_picture(it);
    }

    const auto chunk_ids = m_ca.get_chunk_ids();

    m_type_pick_base = m_ca.m_pick_base;
    for (const auto chunk_id : chunk_ids) {
        const auto &chunk = m_ca.m_chunks.at(chunk_id);

        m_ca.m_vertex_type_picks[{Canvas::VertexType::PICTURE, chunk_id}] = {.offset = m_ca.m_pick_base,
                                                                             .count = chunk.m_pictures.size()};
        m_ca.m_pick_base += chunk.m_pictures.size();
    }
}

} // namespace dune3d
