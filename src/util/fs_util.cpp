#include "fs_util.hpp"

namespace dune3d {

static std::u8string string_to_u8string(const std::string &s)
{
    return std::u8string(s.begin(), s.end());
}

static std::string string_from_u8string(const std::u8string &s)
{
    return std::string(s.begin(), s.end());
}

std::string path_to_string(const std::filesystem::path &path)
{
    return string_from_u8string(path.u8string());
}

std::filesystem::path path_from_string(const std::string &path)
{
    return std::filesystem::path(string_to_u8string(path));
}

} // namespace dune3d
