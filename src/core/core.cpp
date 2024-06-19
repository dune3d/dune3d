#include "core.hpp"
#include "logger/logger.hpp"
#include "util/util.hpp"
#include "nlohmann/json.hpp"
#include "tool_id.hpp"
#include "document/document.hpp"
#include "document/group/group.hpp"
#include "document/group/group_extrude.hpp"
#include "document/entity/entity_workplane.hpp"
#include "system/system.hpp"
#include "util/fs_util.hpp"
#include <iostream>

namespace dune3d {

Core::Core(EditorInterface &intf) : m_intf(intf)
{
}

Core::~Core() = default;

Document &Core::get_current_document()
{
    return get_current_document_info().get_document();
}

const Document &Core::get_current_last_document() const
{
    auto &doc = m_documents.at(m_current_document);
    return doc.get_last_document();
}

UUID Core::add_document()
{
    auto uu = UUID::random();
    m_documents.emplace(std::piecewise_construct, std::forward_as_tuple(uu), std::forward_as_tuple(uu));
    if (m_documents.size() == 1)
        m_current_document = uu;
    m_signal_documents_changed.emit();

    return uu;
}

UUID Core::add_document(const std::filesystem::path &path)
{
    for (const auto &[uu, docinf] : m_documents) {
        if (docinf.m_path == path)
            return uu;
    }
    auto uu = UUID::random();

    m_documents.emplace(std::piecewise_construct, std::forward_as_tuple(uu), std::forward_as_tuple(uu, path));
    if (m_documents.size() == 1)
        m_current_document = uu;
    m_signal_documents_changed.emit();
    return uu;
}

void Core::close_document(const UUID &uu)
{
    m_documents.erase(uu);
    if (m_current_document == uu && m_documents.size()) {
        m_current_document = m_documents.begin()->first;
    }
    m_signal_documents_changed.emit();
}

std::vector<IDocumentInfo *> Core::get_documents()
{
    std::vector<IDocumentInfo *> r;
    r.reserve(m_documents.size());
    for (auto &[uu, docinf] : m_documents) {
        r.push_back(&docinf);
    }
    return r;
}

void Core::undo()
{
    if (!has_documents())
        return;
    if (get_current_document_info().undo()) {
        fix_current_group();
        m_signal_rebuilt.emit();
        m_signal_needs_save.emit();
    }
}

void Core::redo()
{
    if (!has_documents())
        return;
    if (get_current_document_info().redo()) {
        fix_current_group();
        m_signal_rebuilt.emit();
        m_signal_needs_save.emit();
    }
}

bool Core::can_undo() const
{
    if (!has_documents())
        return false;
    return get_current_document_info().m_history_manager.can_undo();
}

bool Core::can_redo() const
{
    if (!has_documents())
        return false;
    return get_current_document_info().m_history_manager.can_redo();
}


Core::DocumentInfo::DocumentInfo(const UUID &uu) : m_uuid(uu)
{
    m_doc.emplace();
    history_push("init");
    m_current_group = m_doc->get_groups_sorted().back()->m_uuid;
}

Core::DocumentInfo::DocumentInfo(const UUID &uu, const std::filesystem::path &path)
    : m_uuid(uu), m_path(path), m_doc(Document::new_from_file(path))
{
    history_push("init");
    m_current_group = m_doc->get_groups_sorted().back()->m_uuid;
}


class HistoryItemDocument : public HistoryManager::HistoryItem {
public:
    HistoryItemDocument(const Document &doc, const std::string &cm) : HistoryManager::HistoryItem(cm), document(doc)
    {
    }
    Document document;
};

const Document &Core::DocumentInfo::get_last_document() const
{
    return dynamic_cast<const HistoryItemDocument &>(m_history_manager.get_current()).document;
}

void Core::DocumentInfo::history_push(const std::string &comment)
{
    m_history_manager.push(std::make_unique<HistoryItemDocument>(m_doc.value(), comment));
}

void Core::DocumentInfo::history_load(const HistoryManager::HistoryItem &it)
{
    auto &itd = dynamic_cast<const HistoryItemDocument &>(it);
    m_doc.reset();
    m_doc.emplace(itd.document);
    m_needs_save = true;
}

void Core::DocumentInfo::revert()
{
    m_doc.reset();
    m_doc.emplace(get_last_document());
}

bool Core::DocumentInfo::undo()
{
    if (!m_history_manager.can_undo())
        return false;
    history_load(m_history_manager.undo());

    return true;
}

bool Core::DocumentInfo::redo()
{
    if (!m_history_manager.can_redo())
        return false;
    history_load(m_history_manager.redo());

    return true;
}

void Core::DocumentInfo::save()
{
    if (is_read_only())
        return;
    if (has_path()) {
        m_doc->m_version.update_file_from_app();
        save_json_to_file(m_path, m_doc->serialize());
        m_needs_save = false;
    }
}

void Core::DocumentInfo::save_as(const std::filesystem::path &path)
{
    m_path = path;
    save();
}

bool Core::DocumentInfo::has_path() const
{
    return m_path != std::filesystem::path();
}

std::string Core::DocumentInfo::get_basename() const
{
    return path_to_string(m_path.filename());
}

UUID Core::DocumentInfo::get_current_workplane() const
{
    if (!m_doc->get_groups().contains(m_current_group))
        return UUID();
    auto &cur_group = m_doc->get_group(m_current_group);
    if (!cur_group.m_active_wrkpl)
        return UUID();
    auto &en = m_doc->get_entity<EntityWorkplane>(cur_group.m_active_wrkpl);
    auto &group = m_doc->get_group<Group>(en.m_group);

    auto &current_group = m_doc->get_group<Group>(m_current_group);
    if (group.get_index() > current_group.get_index())
        return UUID();
    else
        return cur_group.m_active_wrkpl;
}

std::set<InToolActionID> Core::get_tool_actions() const
{
    if (m_tool)
        return m_tool->get_actions();
    else
        return {};
}

std::set<SelectableRef> Core::get_tool_selection() const
{
    if (m_tool)
        return m_tool->m_selection;
    else
        return m_last_tool_selection;
}

Core::CanBeginInfo Core::tool_can_begin(ToolID tool_id, const std::set<SelectableRef> &sel)
{
    if (!has_documents())
        return {ToolBase::CanBegin::NO, false};
    if (is_read_only())
        return {ToolBase::CanBegin::NO, false};
    auto t = create_tool(tool_id);
    t->m_selection = sel;
    auto r = t->can_begin();
    auto s = t->is_specific();
    return {r, s};
}

ToolID Core::get_tool_id() const
{
    if (m_tool)
        return m_tool->get_id();
    else
        return ToolID::NONE;
}

void Core::solve_current(const DraggedList &dragged)
{

    if (!tool_is_active())
        throw std::runtime_error("to be called in tools only");
    auto &doc = get_current_document();
    doc.update_pending(get_current_group(), dragged);
}

Core::ToolStateSetter::ToolStateSetter(ToolState &s, ToolState target) : m_state(s)
{
    if (m_state != ToolState::NONE) {
        /*Logger::log_critical("can't enter tool state " + tool_state_to_string(target) + " in state "
                                     + tool_state_to_string(state),
                             Logger::Domain::TOOL);*/
        m_error = true;
    }
    else {
        m_state = target;
    }
}

std::string Core::ToolStateSetter::tool_state_to_string(ToolState s)
{
    switch (s) {
    case ToolState::BEGINNING:
        return "beginning";
    case ToolState::UPDATING:
        return "updating";
    case ToolState::NONE:
        return "none";
    default:
        return "??";
    }
}

Core::ToolStateSetter::~ToolStateSetter()
{
    if (!m_error)
        m_state = ToolState::NONE;
}

ToolResponse Core::tool_begin(ToolID tool_id, const ToolArgs &args, bool transient)
{
    if (tool_is_active()) {
        throw std::runtime_error("can't begin tool while tool is active");
        return ToolResponse::end();
    }
    ToolStateSetter state_setter{m_tool_state, ToolState::BEGINNING};
    if (state_setter.check_error())
        return ToolResponse::end();

    try {
        m_tool = create_tool(tool_id, transient ? ToolBase::Flags::TRANSIENT : ToolBase::Flags::DEFAULT);
        m_tool->m_selection = args.selection;
        /*
        for (auto [tid, settings] : tool->get_all_settings()) {
            try {
                auto j = s_signal_load_tool_settings.emit(tid);
                if (j != nullptr)
                    settings->load_from_json(j);
                tool->apply_settings();
            }
            catch (const std::exception &e) {
                Logger::log_warning("exception in loading tool setting " + action_catalog.at({ActionID::TOOL,
        tid}).name
                                            + " for tool " + action_catalog.at({ActionID::TOOL, tool_id}).name,
                                    Logger::Domain::CORE, e.what());
            }
        }*/

        /*
        if (!args.keep_selection) {
            tool->selection.clear();
            tool->selection = args.selection;
        }
        if (transient)
            tool->set_transient();
        */
        if (m_tool->can_begin() == ToolBase::CanBegin::NO) { // check if we can actually use this tool
            m_tool.reset();
            return ToolResponse();
        }
    }
    catch (const std::exception &e) {
        Logger::log_critical("exception thrown in tool constructor of "
                             /* + action_catalog.at({ActionID::TOOL, tool_id}).name*/,
                             Logger::Domain::CORE, e.what());
        m_tool.reset();
        return ToolResponse::end();
    }
    if (m_tool) {
        m_current_groups_sorted = get_current_document().get_groups_sorted();
        m_signal_tool_changed.emit();
        ToolResponse r;
        m_pending_begin = false;
        if (!m_tool->needs_delayed_begin())
            return do_begin(args);
        else
            m_pending_begin = true;

        return r;
    }

    return ToolResponse();
}

ToolResponse Core::do_begin(const ToolArgs &args)
{
    ToolResponse r;

    try {
        r = m_tool->begin(args);
    }
    catch (const std::exception &e) {
        m_tool.reset();
        m_signal_tool_changed.emit();
        Logger::log_critical("exception thrown in tool_begin of "
                             /*+ action_catalog.at({ActionID::TOOL, tool_id}).name*/,
                             Logger::Domain::CORE, e.what());
        // history_load(history_manager.get_current());
        rebuild_internal(true, "undo");
        return ToolResponse::end();
    }
    maybe_end_tool(r);
    return r;
}

void Core::set_needs_save(bool v)
{
    if (!has_documents())
        return;
    auto &doci = get_current_document_info();
    if (doci.m_needs_save != v) {
        doci.m_needs_save = v;
        m_signal_needs_save.emit();
    }
    //   if (v)
    // s_signal_modified.emit();

    //  if (v != m_needs_save) {
    //    m_needs_save = v;
    // s_signal_needs_save.emit(v);
    //}
}

void Core::set_needs_save()
{
    set_needs_save(true);
}

bool Core::get_needs_save_any() const
{
    if (!has_documents())
        return false;
    for (const auto &[uu, doc] : m_documents) {
        if (doc.get_needs_save())
            return true;
    }
    return false;
    /// return m_needs_save;
}

bool Core::get_needs_save() const
{
    if (!has_documents())
        return false;

    return get_current_document_info().get_needs_save();
}

bool Core::is_read_only() const
{
    if (!has_documents())
        return false;

    return get_current_document_info().is_read_only();
}

void Core::save_all()
{
    for (auto &[uu, doc] : m_documents) {
        doc.save();
    }
    m_signal_needs_save.emit();
}

void Core::save()
{
    if (!has_documents())
        return;
    get_current_document_info().save();
    m_signal_needs_save.emit();
}

void Core::save_as(const std::filesystem::path &path)
{
    if (!has_documents())
        return;
    get_current_document_info().save_as(path);
    m_signal_needs_save.emit();
}


bool Core::maybe_end_tool(const ToolResponse &r)
{
    if (r.result != ToolResponse::Result::NOP) { // end tool
        /*for (auto [tid, settings] : tool->get_all_settings()) {
            s_signal_save_tool_settings.emit(tid, settings->serialize());
        }*/
        // tool_selection = tool->selection;
        m_last_tool_selection = m_tool->m_selection;
        std::cout << "end tool" << std::endl;
        m_tool.reset();
        m_signal_tool_changed.emit();
        if (r.result == ToolResponse::Result::COMMIT) {
            // const auto comment = action_catalog.at(tool_id_current).name;
            const auto comment = "tool";
            rebuild_internal(false, comment);
            set_needs_save(true);
        }
        else if (r.result == ToolResponse::Result::REVERT) {
            get_current_document_info().revert();
            rebuild_internal(true, "undo");
        }
        else if (r.result == ToolResponse::Result::END) { // did nothing
            // do nothing
        }
        // tool_id_current = ToolID::NONE;
        return true;
    }
    return false;
}

ToolResponse Core::tool_update(ToolArgs &args)
{
    if (m_tool_state != ToolState::NONE) {
        m_pending_tool_args.emplace_back(std::move(args));
        return {};
    }
    ToolStateSetter state_setter{m_tool_state, ToolState::UPDATING};
    if (state_setter.check_error())
        return ToolResponse::end();
    if (m_tool) {
        ToolResponse r;
        if (m_pending_begin) {
            m_pending_begin = false;
            r = do_begin(args);
            if (maybe_end_tool(r))
                return r;
        }

        try {
            r = m_tool->update(args);
        }
        catch (const std::exception &e) {
            m_tool.reset();
            m_signal_tool_changed.emit();
            Logger::log_critical("exception thrown in tool_update", Logger::Domain::CORE, e.what());
            get_current_document_info().revert();
            rebuild_internal(true, "undo");
            return ToolResponse::end();
        }
        maybe_end_tool(r);
        return r;
    }
    return ToolResponse();
}

void Core::fix_current_group()
{
    auto grp = get_current_group();
    if (!get_current_document().get_groups().contains(grp))
        set_current_group(get_current_document().get_groups_sorted().back()->m_uuid);
}

void Core::rebuild_internal(bool from_undo, const std::string &comment)
{
    if (!has_documents())
        return;
    fix_current_group();
    for (auto &[uu, en] : get_current_document().m_entities) {
        en->m_selection_invisible = false;
    }
    get_current_document().update_pending();
    //  frame.expand();
    rebuild_finish(from_undo, comment);
}

void Core::rebuild(const std::string &comment)
{
    rebuild_internal(false, comment);
}

void Core::rebuild_finish(bool from_undo, const std::string &comment)
{

    if (!from_undo) {
        m_documents.at(m_current_document).history_push(comment);
    }
    m_signal_rebuilt.emit();
}

std::optional<ToolArgs> Core::get_pending_tool_args()
{
    if (m_tool_state != ToolState::NONE)
        return {};
    if (m_pending_tool_args.empty())
        return {};
    auto r = std::move(m_pending_tool_args.front());
    m_pending_tool_args.pop_front();
    return r;
}

} // namespace dune3d
