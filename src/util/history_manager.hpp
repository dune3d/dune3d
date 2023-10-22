#pragma once
#include <string>
#include <memory>
#include <deque>
#include "changeable.hpp"

namespace dune3d {
class HistoryManager : public Changeable {
public:
    class HistoryItem {
    public:
        HistoryItem(const std::string &c) : comment(c)
        {
        }
        const std::string comment;
        virtual ~HistoryItem() = default;
    };

    const HistoryItem &undo();
    const HistoryItem &redo();
    const HistoryItem &get_current() const;
    bool has_current() const;

    bool can_undo() const;
    bool can_redo() const;

    const std::string &get_undo_comment() const;
    const std::string &get_redo_comment() const;

    void set_history_max(unsigned int m);
    void set_never_forgets(bool x);

    void push(std::unique_ptr<const HistoryItem> it);
    void clear();

private:
    std::shared_ptr<const HistoryItem> history_current;
    std::deque<std::shared_ptr<const HistoryItem>> undo_stack;
    std::deque<std::shared_ptr<const HistoryItem>> redo_stack;

    unsigned int history_max = 50;
    bool never_forgets = true;
    void trim();
};
} // namespace dune3d
