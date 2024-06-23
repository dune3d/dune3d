#include "preferences.hpp"
#include "util/util.hpp"
#include <fstream>
#include <giomm/file.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include "nlohmann/json.hpp"
#include "logger/logger.hpp"
#include "in_tool_action/in_tool_action_catalog.hpp"
#include "core/tool_id.hpp"
#include "action/action_id.hpp"
#include "action/action.hpp"
#include "canvas/color_palette.hpp"
#include "util/fs_util.hpp"

namespace dune3d {

Preferences::Preferences()
{
}

void Preferences::load_default()
{
    key_sequences.load_from_json(json_from_resource("/org/dune3d/dune3d/preferences/keys_default.json"));
    in_tool_key_sequences.load_from_json(
            json_from_resource("/org/dune3d/dune3d/preferences/in_tool_keys_default.json"));
}

std::filesystem::path Preferences::get_preferences_filename()
{
    return get_config_dir() / "prefs.json";
}

json KeySequencesPreferences::serialize() const
{
    json j;
    for (const auto &[action_tool_id, key_sequenences] : keys) {
        json k;

        if (const auto action = std::get_if<ActionID>(&action_tool_id))
            k["action"] = action_lut.lookup_reverse(*action);
        else if (const auto tool = std::get_if<ToolID>(&action_tool_id))
            k["tool"] = tool_lut.lookup_reverse(*tool);
        k["keys"] = json::array();
        for (const auto &key_sequence : key_sequenences) {
            json seq;
            for (const auto &k : key_sequence) {
                json o;
                o["key"] = gdk_keyval_name(k.key);
                o["mod"] = static_cast<int>(k.mod);
                seq.push_back(o);
            }
            if (seq.size())
                k["keys"].push_back(seq);
        }
        j.push_back(k);
    }
    return j;
}

void KeySequencesPreferences::load_from_json(const json &j)
{
    keys.clear();
    append_from_json(j);
}

void KeySequencesPreferences::append_from_json(const json &j)
{
    for (const auto &it : j) {
        try {
            std::optional<ActionToolID> action_tool_id;
            if (it.contains("action"))
                action_tool_id = action_lut.lookup_opt(it.at("action").get<std::string>());
            else if (it.contains("tool"))
                action_tool_id = tool_lut.lookup_opt(it.at("tool").get<std::string>());
            if (action_tool_id.has_value()) {
                if (keys.count(*action_tool_id) == 0) {
                    for (const auto &it3 : it.at("keys")) {
                        auto &ks = keys[*action_tool_id].emplace_back();
                        for (const auto &it4 : it3) {
                            const auto keyname = it4.at("key").get<std::string>();
                            const auto key = gdk_keyval_from_name(keyname.c_str());
                            const auto mod = static_cast<Gdk::ModifierType>(it4.at("mod").get<int>());
                            ks.push_back({key, mod});
                        }
                    }
                }
            }
        }


        catch (const std::exception &e) {
            Logger::log_warning("error loading key sequence", Logger::Domain::UNSPECIFIED, e.what());
        }
        catch (...) {
            Logger::log_warning("error loading key sequence", Logger::Domain::UNSPECIFIED, "unknown error");
        }
    }
}
json InToolKeySequencesPreferences::serialize() const
{
    json j = json::object();
    for (const auto &[action, sequences] : keys) {
        auto a_str = in_tool_action_lut.lookup_reverse(action);
        for (const auto &it2 : sequences) {
            json seq;
            for (const auto &it4 : it2) {
                json o;
                o["key"] = gdk_keyval_name(it4.key);
                o["mod"] = static_cast<int>(it4.mod);
                seq.push_back(o);
            }
            if (seq.size())
                j[a_str].push_back(seq);
        }
    }
    return j;
}

void InToolKeySequencesPreferences::load_from_json(const json &j)
{
    keys.clear();
    append_from_json(j);
}

void InToolKeySequencesPreferences::append_from_json(const json &j)
{
    for (const auto &[a_str, keys_seqs] : j.items()) {
        try {
            auto action = in_tool_action_lut.lookup(a_str, InToolActionID::NONE);
            if (action != InToolActionID::NONE) {
                if (keys.count(action) == 0) {
                    for (const auto &seq : keys_seqs) {
                        keys[action].emplace_back();
                        for (const auto &it4 : seq) {
                            const auto keyname = it4.at("key").get<std::string>();
                            const auto key = gdk_keyval_from_name(keyname.c_str());
                            const auto mod = static_cast<Gdk::ModifierType>(it4.at("mod").get<int>());
                            keys[action].back().push_back({key, mod});
                        }
                    }
                }
            }
        }
        catch (const std::exception &e) {
            Logger::log_warning("error loading in-tool key sequence", Logger::Domain::UNSPECIFIED, e.what());
        }
        catch (...) {
            Logger::log_warning("error loading int-tool key sequence", Logger::Domain::UNSPECIFIED, "unknown error");
        }
    }
}

json ActionBarPreferences::serialize() const
{
    json j;
    j["enable"] = enable;
    j["remember"] = remember;
    j["show_in_tool"] = show_in_tool;
    return j;
}

void ActionBarPreferences::load_from_json(const json &j)
{
    enable = j.value("enable", true);
    remember = j.value("remember", true);
    show_in_tool = j.value("show_in_tool", true);
}

json ToolBarPreferences::serialize() const
{
    json j;
    j["vertical_layout"] = vertical_layout;
    return j;
}

void ToolBarPreferences::load_from_json(const json &j)
{
    vertical_layout = j.value("vertical_layout", false);
}


#define COLORP_LUT_ITEM(x)                                                                                             \
    {                                                                                                                  \
        #x, ColorP::x                                                                                                  \
    }

static const LutEnumStr<ColorP> colorp_lut = {
        COLORP_LUT_ITEM(ENTITY),
        COLORP_LUT_ITEM(POINT),
        COLORP_LUT_ITEM(CONSTRAINT),
        COLORP_LUT_ITEM(CONSTRUCTION_ENTITY),
        COLORP_LUT_ITEM(CONSTRUCTION_POINT),
        COLORP_LUT_ITEM(BACKGROUND_BOTTOM),
        COLORP_LUT_ITEM(BACKGROUND_TOP),
        COLORP_LUT_ITEM(SOLID_MODEL),
        COLORP_LUT_ITEM(OTHER_BODY_SOLID_MODEL),
        COLORP_LUT_ITEM(INACTIVE_ENTITY),
        COLORP_LUT_ITEM(INACTIVE_POINT),
        COLORP_LUT_ITEM(HOVER),
        COLORP_LUT_ITEM(SELECTED),
        COLORP_LUT_ITEM(SELECTED_HOVER),
        COLORP_LUT_ITEM(HIGHLIGHT),
        COLORP_LUT_ITEM(SELECTION_BOX),
        COLORP_LUT_ITEM(ERROR_OVERLAY),
};

#undef COLORP_LUT_ITEM

static const LutEnumStr<CanvasPreferences::ThemeVariant> theme_variant_lut = {
        {"auto", CanvasPreferences::ThemeVariant::AUTO},
        {"light", CanvasPreferences::ThemeVariant::LIGHT},
        {"dark", CanvasPreferences::ThemeVariant::DARK},
};

static const LutEnumStr<RotationScheme> rotation_scheme_lut = {
        {"default", RotationScheme::DEFAULT},
        {"legacy", RotationScheme::LEGACY},
};

json CanvasPreferences::serialize() const
{
    json j = serialize_colors();
    j["msaa"] = appearance.msaa;
    j["line_width"] = appearance.line_width;
    j["selection_glow"] = appearance.selection_glow;
    j["enable_animations"] = enable_animations;
    j["theme"] = theme;
    j["theme_variant"] = theme_variant_lut.lookup_reverse(theme_variant);
    j["dark_theme"] = dark_theme;
    j["pick_path"] = path_to_string(pick_path);
    j["error_overlay"] = error_overlay;
    j["zoom_to_cursor"] = zoom_to_cursor;
    j["rotation_scheme"] = rotation_scheme_lut.lookup_reverse(rotation_scheme);
    return j;
}

static void to_json(json &j, const Color &c)
{
    j["r"] = c.r;
    j["g"] = c.g;
    j["b"] = c.b;
}


static void from_json(const json &j, Color &c)
{
    c.r = j.at("r").get<float>();
    c.g = j.at("g").get<float>();
    c.b = j.at("b").get<float>();
}

json CanvasPreferences::serialize_colors() const
{
    json j;
    json j_colors = json::object();
    for (const auto &[p, c] : appearance.colors) {
        j_colors[colorp_lut.lookup_reverse(p)] = c;
    }
    j["colors"] = j_colors;
    return j;
}

void CanvasPreferences::load_colors_from_json(const json &j)
{
    if (j.count("colors")) {
        for (const auto &[key, color_j] : j.at("colors").items()) {
            auto c = colorp_lut.lookup(key);
            appearance.colors[c] = color_j.get<Color>();
        }
    }
}

void CanvasPreferences::load_from_json(const json &j)
{
    appearance.msaa = std::max(j.value("msaa", 0), 1);
    appearance.line_width = j.value("line_width", 2.5);
    appearance.selection_glow = j.value("selection_glow", true);
    enable_animations = j.value("enable_animations", true);
    theme = j.value("theme", "Default");
    if (j.contains("theme_variant"))
        theme_variant = theme_variant_lut.lookup(j.at("theme_variant"), ThemeVariant::AUTO);
    dark_theme = j.value("dark_theme", false);
    pick_path = path_from_string(j.value("pick_path", ""));
    error_overlay = j.value("error_overlay", true);
    zoom_to_cursor = j.value("zoom_to_cursor", true);
    if (j.contains("rotation_scheme"))
        rotation_scheme = rotation_scheme_lut.lookup(j.at("rotation_scheme"), RotationScheme::DEFAULT);
    load_colors_from_json(j);
}

json Preferences::serialize() const
{
    json j;
    j["key_sequences"] = key_sequences.serialize();
    j["in_tool_key_sequences"] = in_tool_key_sequences.serialize();
    j["action_bar"] = action_bar.serialize();
    j["tool_bar"] = tool_bar.serialize();
    j["canvas"] = canvas.serialize();
    return j;
}

void Preferences::save()
{
    const auto prefs_filename = get_preferences_filename();

    json j = serialize();
    save_json_to_file(prefs_filename, j);
}

void Preferences::load_from_json(const json &j)
{
    if (j.count("key_sequences"))
        key_sequences.load_from_json(j.at("key_sequences"));
    key_sequences.append_from_json(json_from_resource("/org/dune3d/dune3d/preferences/keys_default.json"));

    if (j.count("in_tool_key_sequences"))
        in_tool_key_sequences.load_from_json(j.at("in_tool_key_sequences"));
    in_tool_key_sequences.append_from_json(
            json_from_resource("/org/dune3d/dune3d/preferences/in_tool_keys_default.json"));

    if (j.count("action_bar"))
        action_bar.load_from_json(j.at("action_bar"));
    if (j.count("canvas"))
        canvas.load_from_json(j.at("canvas"));

    if (j.count("tool_bar"))
        tool_bar.load_from_json(j.at("tool_bar"));
}

void Preferences::load()
{
    auto prefs_filename = get_preferences_filename();
    if (fs::exists(prefs_filename)) {
        json j = load_json_from_file(prefs_filename);
        load_from_json(j);
    }
    else {
        load_default();
    }
    m_signal_changed.emit();
}
} // namespace dune3d
