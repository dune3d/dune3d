#include "key_util.hpp"

namespace dune3d {

void remap_keys(guint &keyval, Gdk::ModifierType &state)
{
#ifdef __APPLE__
    state &= (Gdk::ModifierType)GDK_MODIFIER_MASK;
    if ((int)(state & Gdk::ModifierType::CONTROL_MASK)) {
        state &= ~Gdk::ModifierType::CONTROL_MASK;
        state |= Gdk::ModifierType::ALT_MASK;
    }
    if ((int)(state & Gdk::ModifierType::META_MASK)) {
        state &= ~Gdk::ModifierType::META_MASK;
        state |= Gdk::ModifierType::CONTROL_MASK;
    }
    if (keyval == GDK_KEY_BackSpace)
        keyval = GDK_KEY_Delete;
    else if (keyval == GDK_KEY_Delete)
        keyval = GDK_KEY_BackSpace;
#endif
}

} // namespace dune3d
