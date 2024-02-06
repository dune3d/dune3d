#pragma once
#include "nlohmann/json_fwd.hpp"
#include <sigc++/sigc++.h>
#include <string>
#include <filesystem>
#include "action/action_catalog.hpp"
#include "util/changeable.hpp"
#include "canvas/appearance.hpp"

namespace dune3d {
using json = nlohmann::json;
namespace fs = std::filesystem;

enum class InToolActionID;


class CanvasPreferences {
public:
    Appearance appearance;

    bool enable_animations = true;
    bool dark_theme = false;
    std::string theme = "Default";
    enum class ThemeVariant { AUTO, DARK, LIGHT };
    ThemeVariant theme_variant = ThemeVariant::AUTO;
    fs::path pick_path;

    void load_from_json(const json &j);
    void load_colors_from_json(const json &j);
    json serialize() const;
    json serialize_colors() const;
};

class KeySequencesPreferences {
public:
    std::map<ActionToolID, std::vector<KeySequence>> keys;

    void load_from_json(const json &j);
    void append_from_json(const json &j);
    json serialize() const;
};

class InToolKeySequencesPreferences {
public:
    std::map<InToolActionID, std::vector<KeySequence>> keys;

    void load_from_json(const json &j);
    void append_from_json(const json &j);
    json serialize() const;
};

class ActionBarPreferences {
public:
    bool enable = true;
    bool remember = true;
    bool show_in_tool = true;

    void load_from_json(const json &j);
    json serialize() const;
};

class ToolBarPreferences {
public:
    bool vertical_layout = false;

    void load_from_json(const json &j);
    json serialize() const;
};

class Preferences : public Changeable {
public:
    static const Preferences &get();

    Preferences();
    void set_filename(const fs::path &filename);
    void load();
    void load_default();
    void load_from_json(const json &j);
    void save();
    static fs::path get_preferences_filename();
    json serialize() const;

    KeySequencesPreferences key_sequences;

    ActionBarPreferences action_bar;
    InToolKeySequencesPreferences in_tool_key_sequences;
    ToolBarPreferences tool_bar;
    CanvasPreferences canvas;

private:
    fs::path filename;
};
} // namespace dune3d
