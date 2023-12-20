#pragma once
#include <filesystem>
#include <set>
#include "util/uuid.hpp"
#include <functional>

namespace dune3d {
class Document;
class Group;
void export_paths(const std::filesystem::path &filename, const Document &doc, const UUID &group_uu,
                  std::function<bool(const Group &)> group_filter = nullptr);
} // namespace dune3d
