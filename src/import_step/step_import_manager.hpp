#pragma once
#include <memory>
#include <mutex>
#include <map>
#include "imported_step.hpp"

namespace dune3d {

class STEPImportManager {
public:
    static STEPImportManager &get();
    std::shared_ptr<ImportedSTEP> import_step(const std::filesystem::path &path);

private:
    STEPImportManager();
    std::map<std::filesystem::path, std::shared_ptr<ImportedSTEP>> m_imported;
};

} // namespace dune3d
