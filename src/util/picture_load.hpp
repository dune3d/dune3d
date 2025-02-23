#pragma once
#include <filesystem>

namespace dune3d {

class Document;

void pictures_load(Document &doc, const std::filesystem::path &dir);
void pictures_save(const Document &doc, const std::filesystem::path &dir);

} // namespace dune3d
