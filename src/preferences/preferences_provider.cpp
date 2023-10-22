#include "preferences_provider.hpp"
#include "preferences.hpp"
#include <assert.h>

namespace dune3d {
static PreferencesProvider the_preferences_provider;

PreferencesProvider::PreferencesProvider()
{
}

PreferencesProvider &PreferencesProvider::get()
{
    return the_preferences_provider;
}

void PreferencesProvider::set_prefs(Preferences &p)
{
    assert(prefs == nullptr);
    prefs = &p;
    prefs->signal_changed().connect([this] { m_signal_changed.emit(); });
}

const Preferences &PreferencesProvider::get_prefs_ns() const
{
    assert(prefs);
    return *prefs;
}

const Preferences &PreferencesProvider::get_prefs()
{
    return the_preferences_provider.get_prefs_ns();
}

} // namespace dune3d
