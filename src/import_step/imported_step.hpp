#pragma once
#include "import.hpp"
#include <filesystem>
#include <atomic>

namespace dune3d {
class ImportedSTEP {
public:
    ImportedSTEP(const std::filesystem::path &p) : path(p)
    {
    }

    std::atomic_bool ready;
    const std::filesystem::path path;
    STEPImporter::Result result;
};
} // namespace dune3d
