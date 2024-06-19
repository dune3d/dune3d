#pragma once
#include "util/uuid.hpp"
#include <filesystem>

namespace dune3d {

class IDocumentInfo;

class IDocumentProvider {
public:
    virtual IDocumentInfo &get_idocument_info(const UUID &uu) = 0;
    virtual IDocumentInfo *get_idocument_info_by_path(const std::filesystem::path &path) = 0;
};

} // namespace dune3d
