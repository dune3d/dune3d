#pragma once
#include <filesystem>

namespace dune3d {

std::string path_to_string(const std::filesystem::path &path);
std::filesystem::path path_from_string(const std::string &path);

} // namespace dune3d
