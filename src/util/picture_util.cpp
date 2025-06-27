#include "picture_util.hpp"
#include "picture_data.hpp"
#include "fs_util.hpp"
#include <gdkmm/texture.h>

namespace dune3d {
std::shared_ptr<const PictureData> picture_data_from_file(const std::filesystem::path &filename)
{
    auto tex = Gdk::Texture::create_from_filename(path_to_string(filename));
    return picture_data_from_texture(tex);
}

std::shared_ptr<const class PictureData> picture_data_from_texture(Glib::RefPtr<Gdk::Texture> texture)
{
    std::vector<uint32_t> v;
    auto w = texture->get_width();
    auto h = texture->get_height();
    v.resize(w * h);
    std::vector<guchar> buf(w * h * 4);
    auto px = buf.data();
    texture->download(px, w * 4);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            auto p = px + (x * 4);
            v[y * w + x] = (p[2]) | (p[1] << 8) | (p[0] << 16) | (p[3] << 24);
        }
        px += w * 4;
    }
    return std::make_shared<PictureData>(UUID::random(), w, h, std::move(v));
}
} // namespace dune3d
