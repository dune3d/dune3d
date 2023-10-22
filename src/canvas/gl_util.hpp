#include <epoxy/gl.h>
#include <string>
#include <sstream>
#include <array>
#include <glm/fwd.hpp>

namespace dune3d {
// GLuint gl_create_shader (int type, char *src);
GLuint gl_create_program_from_resource(const char *vertex_resource, const char *fragment_resource,
                                       const char *geometry_resource);

void gl_show_error(const std::string &s);
void gl_color_to_uniform_3f(GLuint loc, const class Color &c);
void gl_color_to_uniform_4f(GLuint loc, const class Color &c, float alpha = 1);
GLint gl_clamp_samples(GLint samples);

void gl_mat3_to_array(std::array<float, 12> &dest, const glm::mat3 &src);

#define GET_LOC(d, loc)                                                                                                \
    do {                                                                                                               \
        d->m_##loc##_loc = glGetUniformLocation(d->m_program, #loc);                                                   \
    } while (0);

#define GET_LOC2(d, loc)                                                                                               \
    do {                                                                                                               \
        (d).loc##_loc = glGetUniformLocation((d).program, #loc);                                                       \
    } while (0);

#define GL_CHECK_ERROR                                                                                                 \
    if (int e = glGetError()) {                                                                                        \
        std::stringstream ss;                                                                                          \
        ss << "gl error " << e << " in " << __FILE__ << ":" << __LINE__;                                               \
        gl_show_error(ss.str());                                                                                       \
        abort();                                                                                                       \
    }
} // namespace dune3d
