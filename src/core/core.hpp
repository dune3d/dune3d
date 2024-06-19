#pragma once
#include "document/document.hpp"
#include "icore.hpp"
#include "util/history_manager.hpp"
#include <filesystem>
#include <optional>
#include <sigc++/sigc++.h>
#include "tool.hpp"
#include "idocument_info.hpp"

namespace dune3d {

class EditorInterface;

class Core : public ICore {
public:
    explicit Core(EditorInterface &intf);

    bool has_documents() const override
    {
        return m_documents.size();
    }
    Document &get_current_document() override;
    const Document &get_current_last_document() const override;

    UUID add_document();
    UUID add_document(const std::filesystem::path &path);
    void close_document(const UUID &uu);

    using type_signal_documents_changed = sigc::signal<void()>;
    type_signal_documents_changed signal_documents_changed()
    {
        return m_signal_documents_changed;
    }

    using type_signal_tool_changed = sigc::signal<void()>;
    type_signal_tool_changed signal_tool_changed()
    {
        return m_signal_tool_changed;
    }

    using type_signal_rebuilt = sigc::signal<void()>;
    type_signal_rebuilt signal_rebuilt()
    {
        return m_signal_rebuilt;
    }

    using type_signal_needs_save = sigc::signal<void()>;
    type_signal_needs_save signal_needs_save()
    {
        return m_signal_needs_save;
    }

    ToolResponse tool_begin(ToolID tool_id, const ToolArgs &args, bool transient = false);
    ToolResponse tool_update(ToolArgs &args);

    std::set<InToolActionID> get_tool_actions() const;

    struct CanBeginInfo {
        bool get_can_begin() const
        {
            return can_begin != ToolBase::CanBegin::NO;
        }
        ToolBase::CanBegin can_begin;
        bool is_specific;
    };

    CanBeginInfo tool_can_begin(ToolID tool_id, const std::set<SelectableRef> &sel);

    inline bool tool_is_active() const
    {
        return m_tool != nullptr;
    }

    ToolID get_tool_id() const;

    std::set<SelectableRef> get_tool_selection() const;

    bool get_needs_save_any() const;
    bool get_needs_save() const;
    bool is_read_only() const;
    void set_needs_save();

    void save_all();
    void save();
    void save_as(const std::filesystem::path &path);

    void rebuild(const std::string &comment);

    void undo();
    void redo();

    bool can_undo() const;
    bool can_redo() const;

    std::optional<ToolArgs> get_pending_tool_args();

    ~Core();


    std::vector<IDocumentInfo *> get_documents();
    IDocumentInfo &get_idocument_info(const UUID &uu)
    {
        return m_documents.at(uu);
    }

    IDocumentInfo &get_current_idocument_info()
    {
        return m_documents.at(m_current_document);
    }

    UUID get_current_group() const override
    {
        return get_current_document_info().get_current_group();
    }

    void set_current_group(const UUID &uu)
    {
        if (get_current_document().get_groups().contains(uu))
            get_current_document_info().m_current_group = uu;
    }
    void set_current_document(const UUID &uu)
    {
        if (m_documents.contains(uu))
            m_current_document = uu;
    }

    UUID get_current_workplane() const override
    {
        return get_current_document_info().get_current_workplane();
    }

    std::filesystem::path get_current_document_directory() const override
    {
        return get_current_document_info().m_path.parent_path();
    }

    void solve_current(const DraggedList &dragged) override;


private:
    EditorInterface &m_intf;

    class DocumentInfo : public IDocumentInfo {
    public:
        explicit DocumentInfo(const UUID &uu);
        explicit DocumentInfo(const UUID &uu, const std::filesystem::path &path);
        bool undo();
        bool redo();

        void history_load(const HistoryManager::HistoryItem &it);
        void history_push(const std::string &comment);
        void revert();
        void save();
        void save_as(const std::filesystem::path &path);
        bool has_path() const override;
        Document &get_document() override
        {
            return m_doc.value();
        }
        const Document &get_document() const override
        {
            return m_doc.value();
        }
        bool get_needs_save() const override
        {
            return m_needs_save && !is_read_only();
        }

        bool is_read_only() const override
        {
            auto &ver = m_doc->m_version;
            return ver.get_app() < ver.get_file();
        }

        const Document &get_last_document() const;
        std::string get_basename() const override;

        UUID get_current_group() const override
        {
            return m_current_group;
        }

        UUID get_current_workplane() const override;

        UUID get_uuid() const override
        {
            return m_uuid;
        }
        const UUID m_uuid;

        std::filesystem::path m_path;
        std::optional<Document> m_doc;
        bool m_needs_save = false;
        UUID m_current_group;
        HistoryManager m_history_manager;
    };

    DocumentInfo &get_current_document_info()
    {
        return m_documents.at(m_current_document);
    }
    const DocumentInfo &get_current_document_info() const
    {
        return m_documents.at(m_current_document);
    }


    std::map<UUID, DocumentInfo> m_documents;

    UUID m_current_document;

    type_signal_documents_changed m_signal_documents_changed;
    type_signal_tool_changed m_signal_tool_changed;
    type_signal_rebuilt m_signal_rebuilt;
    type_signal_needs_save m_signal_needs_save;

    void rebuild_internal(bool from_undo, const std::string &comment);
    void rebuild_finish(bool from_undo, const std::string &comment);

    void set_needs_save(bool v);

    std::unique_ptr<ToolBase> create_tool(ToolID tool_id, ToolBase::Flags flags = ToolBase::Flags::DEFAULT);
    std::unique_ptr<ToolBase> m_tool = nullptr;
    bool maybe_end_tool(const ToolResponse &r);
    std::set<SelectableRef> m_last_tool_selection;
    bool m_pending_begin = false;
    ToolResponse do_begin(const ToolArgs &args);

    enum class ToolState { NONE, BEGINNING, UPDATING };
    ToolState m_tool_state = ToolState::NONE;

    std::list<ToolArgs> m_pending_tool_args;

    std::vector<Group *> m_current_groups_sorted;

    void fix_current_group();

    class ToolStateSetter {
    public:
        ToolStateSetter(ToolState &s, ToolState target);
        bool check_error() const
        {
            return m_error;
        }
        ~ToolStateSetter();

    private:
        ToolState &m_state;
        bool m_error = false;
        static std::string tool_state_to_string(ToolState s);
    };
};

} // namespace dune3d
