#pragma once

namespace dune3d {

class DocumentView;

class IDocumentViewProvider {
public:
    virtual DocumentView &get_current_document_view() = 0;
};
} // namespace dune3d
