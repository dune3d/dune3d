#pragma once
#include <pangomm.h>
#include "util/uuid.hpp"

namespace dune3d {
class EntityText;
class Document;

void render_text(EntityText &text, Glib::RefPtr<Pango::Context> ctx, const Document &doc);

} // namespace dune3d
