#pragma once
#include "util/uuid.hpp"
#include <filesystem>

namespace dune3d {

class Document;

class IDocumentInfo {
public:
    virtual Document &get_document() = 0;
    virtual const Document &get_document() const = 0;
    virtual std::string get_basename() const = 0;
    virtual std::filesystem::path get_dirname() const = 0;
    virtual std::filesystem::path get_path() const = 0;
    virtual UUID get_current_group() const = 0;
    virtual UUID get_current_workplane() const = 0;
    virtual UUID get_uuid() const = 0;
    virtual bool has_path() const = 0;
    virtual bool get_needs_save() const = 0;
    virtual bool is_read_only() const = 0;
    virtual bool can_close() const = 0;
};

} // namespace dune3d
