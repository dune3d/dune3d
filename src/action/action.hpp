#pragma once
#include <string>
#include <vector>
#include <gdkmm.h>
#include <functional>
#include <variant>

namespace dune3d {

enum class ToolID;
enum class ActionID;

using ActionToolID = std::variant<ActionID, ToolID>;

enum class ActionGroup {
    ALL,
    UNKNOWN,
    DRAW,
    CONSTRAIN,
    MEASURE,
    UNDO,
    MOVE,
    DOCUMENT,
    GROUP,
    VIEW,
    CLIPBOARD,
};

enum class ActionSource {
    UNKNOWN,
    KEY,
};

struct KeySequenceItem {
    unsigned int key;
    Gdk::ModifierType mod;
    friend bool operator==(const KeySequenceItem &, const KeySequenceItem &) = default;
};
using KeySequence = std::vector<KeySequenceItem>;

std::string key_sequence_item_to_string(const KeySequenceItem &it);
std::string key_sequence_to_string(const KeySequence &keys);
std::string key_sequence_to_string_short(const KeySequence &keys);

std::string key_sequences_to_string(const std::vector<KeySequence> &seqs);

enum class KeyMatchResult { NONE, PREFIX, COMPLETE };
KeyMatchResult key_sequence_match(const KeySequence &keys_current, const KeySequence &keys_from_action);

class ActionConnection {
public:
    ActionConnection(ActionToolID atid, std::function<void(const ActionConnection &, ActionSource)> c) : id(atid), cb(c)
    {
    }

    const ActionToolID id;
    std::vector<KeySequence> key_sequences;
    std::function<void(const ActionConnection &, ActionSource)> cb;
};

} // namespace dune3d
