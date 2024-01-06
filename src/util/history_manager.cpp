#include "history_manager.hpp"
#include <stdexcept>
#include <assert.h>
#include <set>
#include <ranges>

namespace dune3d {
void HistoryManager::clear()
{
    undo_stack.clear();
    redo_stack.clear();
    history_current.reset();
    m_signal_changed.emit();
}

static size_t count_unique(const std::deque<std::shared_ptr<const HistoryManager::HistoryItem>> &items)
{
    std::set<const HistoryManager::HistoryItem *> unique_items;
    for (const auto &x : items) {
        unique_items.insert(x.get());
    }
    return unique_items.size();
}

void HistoryManager::trim()
{
    while (count_unique(undo_stack) > history_max) {
        if (undo_stack.front().use_count() > 1)
            return;
        undo_stack.pop_front();
    }
}

void HistoryManager::set_history_max(unsigned int m)
{
    history_max = m;
}

void HistoryManager::set_never_forgets(bool x)
{
    never_forgets = x;
}

bool HistoryManager::can_redo() const
{
    return redo_stack.size();
}

bool HistoryManager::can_undo() const
{
    return undo_stack.size() && history_current;
}

static const std::string empty_string;

const std::string &HistoryManager::get_undo_comment() const
{
    if (can_undo()) {
        return history_current->comment;
    }
    else {
        return empty_string;
    }
}

const std::string &HistoryManager::get_redo_comment() const
{
    if (can_redo()) {
        return redo_stack.back()->comment;
    }
    else {
        return empty_string;
    }
}

const HistoryManager::HistoryItem &HistoryManager::undo()
{
    if (!can_undo())
        throw std::runtime_error("can't undo");
    redo_stack.push_back(history_current);
    history_current = undo_stack.back();
    undo_stack.pop_back();
    m_signal_changed.emit();
    return get_current();
}

const HistoryManager::HistoryItem &HistoryManager::redo()
{
    if (!can_redo())
        throw std::runtime_error("can't redo");
    undo_stack.push_back(history_current);
    history_current = redo_stack.back();
    redo_stack.pop_back();
    m_signal_changed.emit();
    return get_current();
}

const HistoryManager::HistoryItem &HistoryManager::get_current() const
{
    return *history_current;
}

bool HistoryManager::has_current() const
{
    return history_current.get();
}

void HistoryManager::push(std::unique_ptr<const HistoryItem> it)
{
    if (history_current)
        undo_stack.push_back(history_current);

    if (redo_stack.size()) {
        if (never_forgets) {
            for (auto &x : redo_stack | std::views::reverse) {
                undo_stack.push_back(x);
            }
            undo_stack.push_back(history_current);
        }
        redo_stack.clear();
    }
    history_current = std::move(it);
    trim();
    m_signal_changed.emit();
}

} // namespace dune3d
