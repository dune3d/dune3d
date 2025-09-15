#pragma once
#include <filesystem>
#include <set>
#include "util/uuid.hpp"
#include <functional>

namespace dune3d {
class Document;
class Group;
void export_dxf(const std::filesystem::path &filename, const Document &doc, const UUID &group_uu);
} // namespace dune3d
