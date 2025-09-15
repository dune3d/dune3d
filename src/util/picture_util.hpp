#pragma once
#include <memory>
#include <filesystem>
#include <gdkmm.h>

namespace dune3d {
std::shared_ptr<const class PictureData> picture_data_from_file(const std::filesystem::path &filename);
std::shared_ptr<const class PictureData> picture_data_from_texture(Glib::RefPtr<Gdk::Texture> tex);
} // namespace dune3d
