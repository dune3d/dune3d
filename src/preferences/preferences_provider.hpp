#pragma once
#include "util/changeable.hpp"

namespace dune3d {
class PreferencesProvider : public Changeable {
public:
    PreferencesProvider();
    static PreferencesProvider &get();
    const class Preferences &get_prefs_ns() const;
    static const class Preferences &get_prefs();
    void set_prefs(Preferences &p);

private:
    class Preferences *prefs = nullptr;
};

} // namespace dune3d
