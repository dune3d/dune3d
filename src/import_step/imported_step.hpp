#pragma once
#include "import.hpp"
#include <filesystem>
#include <atomic>

namespace dune3d {
class ImportedSTEP {
public:
    ImportedSTEP(const std::filesystem::path &p, const std::string &h) : path(p), hash(h)
    {
    }

    std::atomic_bool ready;
    const std::filesystem::path path;
    const std::string hash;
    STEPImporter::Result result;
};
} // namespace dune3d
