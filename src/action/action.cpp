#include "action.hpp"
#include "action_id.hpp"
#include "util/str_util.hpp"
#include <gdkmm.h>
#include "core/tool_id.hpp"

namespace dune3d {

std::string key_sequence_item_to_string(const KeySequenceItem &it)
{
    std::string txt;
    std::string keyname(gdk_keyval_name(it.key));
    if ((int)(it.mod & Gdk::ModifierType::CONTROL_MASK)) {
        txt += "Ctrl+";
    }
    if ((int)(it.mod & Gdk::ModifierType::SHIFT_MASK)) {
        txt += "Shift+";
    }
    if ((int)(it.mod & Gdk::ModifierType::ALT_MASK)) {
        txt += "Alt+";
    }
    txt += keyname;
    return txt;
}

std::string key_sequence_to_string(const KeySequence &keys)
{
    std::string txt;
    for (const auto &it : keys) {
        txt += key_sequence_item_to_string(it);
        txt += " ";
    }
    rtrim(txt);
    return txt;
}

static std::string keyval_to_string(unsigned int kv)
{
    switch (kv) {
    case GDK_KEY_slash:
        return "/";
    case GDK_KEY_Return:
        return "⏎";
    case GDK_KEY_space:
        return "␣";
    case GDK_KEY_plus:
        return "+";
    case GDK_KEY_minus:
        return "−";
    case GDK_KEY_comma:
        return ",";
    case GDK_KEY_period:
        return ".";
    case GDK_KEY_less:
        return "<";
    case GDK_KEY_greater:
        return ">";
    case GDK_KEY_BackSpace:
        return "⌫";
    default:
        return gdk_keyval_name(kv);
    }
}

std::string key_sequence_to_string_short(const KeySequence &keys)
{
    if (keys.size() == 1 && keys.front().mod == static_cast<Gdk::ModifierType>(0)) {
        return keyval_to_string(keys.front().key);
    }
    else {
        return key_sequence_to_string(keys);
    }
}

std::string key_sequences_to_string(const std::vector<KeySequence> &seqs)
{
    std::string s;
    for (const auto &it : seqs) {
        if (s.size()) {
            s += ", ";
        }
        s += key_sequence_to_string(it);
    }
    return s;
}

KeyMatchResult key_sequence_match(const KeySequence &keys_current, const KeySequence &keys_from_action)
{
    const auto minl = std::min(keys_current.size(), keys_from_action.size());
    const bool match = minl && std::equal(keys_current.begin(), keys_current.begin() + minl, keys_from_action.begin());
    if (!match)
        return KeyMatchResult::NONE;
    else if (keys_current.size() == keys_from_action.size())
        return KeyMatchResult::COMPLETE;
    else
        return KeyMatchResult::PREFIX;
}

} // namespace dune3d
