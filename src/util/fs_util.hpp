#pragma once
#include <filesystem>
#include <optional>

namespace dune3d {

std::string path_to_string(const std::filesystem::path &path);
std::filesystem::path path_from_string(const std::string &path);
std::optional<std::filesystem::path> get_relative_filename(const std::filesystem::path &path,
                                                           const std::filesystem::path &base);

} // namespace dune3d
