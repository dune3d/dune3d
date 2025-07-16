#include "group_editor.hpp"
#include "document/group/group.hpp"
#include "document/group/group_extrude.hpp"
#include "document/group/group_lathe.hpp"
#include "document/group/group_revolve.hpp"
#include "document/group/group_local_operation.hpp"
#include "document/group/group_reference.hpp"
#include "document/group/group_linear_array.hpp"
#include "document/group/group_polar_array.hpp"
#include "document/group/group_loft.hpp"
#include "document/group/group_mirror.hpp"
#include "document/group/group_solid_model_operation.hpp"
#include "document/group/group_clone.hpp"
#include "document/group/group_pipe.hpp"
#include "document/entity/entity.hpp"
#include "widgets/spin_button_dim.hpp"
#include "widgets/select_groups_dialog.hpp"
#include "widgets/select_group_dialog.hpp"
#include "core/core.hpp"
#include "core/tool_id.hpp"
#include "util/gtk_util.hpp"
#include "action/action_id.hpp"
#include <format>

namespace dune3d {

class GroupEditorSolidModel : public GroupEditor {
protected:
    using GroupEditor::GroupEditor;
    void add_operation_combo()
    {
        auto &group = get_group();
        auto items = Gtk::StringList::create();
        items->append("Union");
        items->append("Difference");
        items->append("Intersection");

        m_operation_combo = Gtk::make_managed<Gtk::DropDown>(items);
        m_operation_combo->set_selected(static_cast<guint>(group.get_operation()));
        m_operation_combo->property_selected().signal_changed().connect([this] {
            if (is_reloading())
                return;
            auto &group = get_group();
            group.set_operation(static_cast<GroupExtrude::Operation>(m_operation_combo->get_selected()));
            m_core.get_current_document().set_group_update_solid_model_pending(m_group_uu);
            m_signal_changed.emit(CommitMode::IMMEDIATE);
        });
        grid_attach_label_and_widget(*this, "Operation", *m_operation_combo, m_top);
    }


    void do_reload() override
    {
        GroupEditor::do_reload();
        auto &group = get_group();
        m_operation_combo->set_selected(static_cast<guint>(group.get_operation()));
    }

private:
    IGroupSolidModel &get_group()
    {
        return m_core.get_current_document().get_group<IGroupSolidModel>(m_group_uu);
    }

    Gtk::DropDown *m_operation_combo = nullptr;
};

class GroupEditorSweep : public GroupEditorSolidModel {
protected:
    using GroupEditorSolidModel::GroupEditorSolidModel;
    void add_operation_combo()
    {
        GroupEditorSolidModel::add_operation_combo();

        auto source_group_name = m_core.get_current_document().get_group(get_group().m_source_group).m_name;
        auto source_label = Gtk::make_managed<Gtk::Label>(source_group_name);
        source_label->set_xalign(0);
        grid_attach_label_and_widget(*this, "Source group", *source_label, m_top);
    }

    void do_reload() override
    {
        GroupEditorSolidModel::do_reload();
    }

private:
    GroupSweep &get_group()
    {
        return m_core.get_current_document().get_group<GroupSweep>(m_group_uu);
    }
};

class GroupEditorSketch : public GroupEditorSolidModel {
public:
    GroupEditorSketch(Core &core, const UUID &group_uu) : GroupEditorSolidModel(core, group_uu)
    {
        add_operation_combo();
    }
};

class GroupEditorExtrude : public GroupEditorSweep {
public:
    GroupEditorExtrude(Core &core, const UUID &group_uu) : GroupEditorSweep(core, group_uu)
    {
        m_normal_switch = Gtk::make_managed<Gtk::Switch>();
        m_normal_switch->set_valign(Gtk::Align::CENTER);
        m_normal_switch->set_halign(Gtk::Align::START);
        auto &group = get_group();
        m_normal_switch->set_active(group.m_direction == GroupExtrude::Direction::NORMAL);
        grid_attach_label_and_widget(*this, "Along normal", *m_normal_switch, m_top);
        m_normal_switch->property_active().signal_changed().connect([this] {
            if (is_reloading())
                return;
            auto &group = get_group();
            if (m_normal_switch->get_active())
                group.m_direction = GroupExtrude::Direction::NORMAL;
            else
                group.m_direction = GroupExtrude::Direction::ARBITRARY;
            m_core.get_current_document().set_group_solve_pending(group.m_uuid);
            m_signal_changed.emit(CommitMode::IMMEDIATE);
        });
        add_operation_combo();
        {
            auto items = Gtk::StringList::create();
            items->append("Single");
            items->append("Offset");
            items->append("Offset symmetric");

            m_mode_combo = Gtk::make_managed<Gtk::DropDown>(items);
            m_mode_combo->set_selected(static_cast<guint>(group.m_mode));
            m_mode_combo->property_selected().signal_changed().connect([this] {
                if (is_reloading())
                    return;
                auto &group = get_group();
                group.m_mode = static_cast<GroupExtrude::Mode>(m_mode_combo->get_selected());
                m_core.get_current_document().set_group_generate_pending(group.m_uuid);
                m_signal_changed.emit(CommitMode::IMMEDIATE);
            });
            grid_attach_label_and_widget(*this, "Mode", *m_mode_combo, m_top);
        }
    }

    void do_reload() override
    {
        GroupEditorSweep::do_reload();
        auto &group = get_group();
        m_normal_switch->set_active(group.m_direction == GroupExtrude::Direction::NORMAL);
        m_mode_combo->set_selected(static_cast<guint>(group.m_mode));
    }

private:
    GroupExtrude &get_group()
    {
        return m_core.get_current_document().get_group<GroupExtrude>(m_group_uu);
    }

    Gtk::Switch *m_normal_switch = nullptr;
    Gtk::DropDown *m_mode_combo = nullptr;
};

class GroupEditorLathe : public GroupEditorSweep {
public:
    GroupEditorLathe(Core &core, const UUID &group_uu) : GroupEditorSweep(core, group_uu)
    {
        add_operation_combo();
    }
};

class GroupEditorMirror : public GroupEditor {
public:
    GroupEditorMirror(Core &core, const UUID &group_uu) : GroupEditor(core, group_uu)
    {

        m_include_source_switch = Gtk::make_managed<Gtk::Switch>();
        m_include_source_switch->set_valign(Gtk::Align::CENTER);
        m_include_source_switch->set_halign(Gtk::Align::START);
        auto &group = get_group();
        m_include_source_switch->set_active(group.m_include_source);
        grid_attach_label_and_widget(*this, "Include source", *m_include_source_switch, m_top);
        m_include_source_switch->property_active().signal_changed().connect([this] {
            if (is_reloading())
                return;
            auto &group = get_group();
            group.m_include_source = m_include_source_switch->get_active();
            m_core.get_current_document().set_group_generate_pending(group.m_uuid);
            m_signal_changed.emit(CommitMode::IMMEDIATE);
        });
    }

private:
    Gtk::Switch *m_include_source_switch = nullptr;

    void do_reload() override
    {
        GroupEditor::do_reload();
        auto &group = get_group();
        m_include_source_switch->set_active(group.m_include_source);
    }

    GroupMirror &get_group()
    {
        return m_core.get_current_document().get_group<GroupMirror>(m_group_uu);
    }
};

class GroupEditorLoft : public GroupEditorSolidModel {
public:
    GroupEditorLoft(Core &core, const UUID &group_uu) : GroupEditorSolidModel(core, group_uu)
    {
        add_operation_combo();

        m_ruled_switch = Gtk::make_managed<Gtk::Switch>();
        m_ruled_switch->set_valign(Gtk::Align::CENTER);
        m_ruled_switch->set_halign(Gtk::Align::START);
        auto &group = get_group();
        m_ruled_switch->set_active(group.m_ruled);
        grid_attach_label_and_widget(*this, "Ruled", *m_ruled_switch, m_top);
        m_ruled_switch->property_active().signal_changed().connect([this] {
            if (is_reloading())
                return;
            auto &group = get_group();
            group.m_ruled = m_ruled_switch->get_active();
            m_core.get_current_document().set_group_update_solid_model_pending(group.m_uuid);
            m_signal_changed.emit(CommitMode::IMMEDIATE);
        });

        m_source_groups_button = Gtk::make_managed<Gtk::Button>();
        m_source_groups_button->signal_clicked().connect(sigc::mem_fun(*this, &GroupEditorLoft::edit_source_groups));
        m_source_groups_button_label = Gtk::make_managed<Gtk::Label>();
        m_source_groups_button_label->set_ellipsize(Pango::EllipsizeMode::END);
        m_source_groups_button_label->set_xalign(0);
        m_source_groups_button->set_child(*m_source_groups_button_label);
        grid_attach_label_and_widget(*this, "Source groups", *m_source_groups_button, m_top);
        update_source_groups_label(group);
    }

private:
    Gtk::Switch *m_ruled_switch = nullptr;
    Gtk::Button *m_source_groups_button = nullptr;
    Gtk::Label *m_source_groups_button_label = nullptr;

    void do_reload() override
    {
        GroupEditorSolidModel::do_reload();
        auto &group = get_group();
        m_ruled_switch->set_active(group.m_ruled);
        update_source_groups_label(group);
    }

    void update_source_groups_label(const GroupLoft &group)
    {
        std::string s;
        for (const auto &src : group.m_sources) {
            auto &gr = m_core.get_current_document().get_group(src.group);
            if (s.size())
                s += ", ";
            s += gr.m_name;
        }
        m_source_groups_button_label->set_text(s);
    }

    void edit_source_groups()
    {
        std::vector<UUID> current_source_groups;
        auto &group = get_group();
        for (const auto &src : group.m_sources) {
            current_source_groups.push_back(src.group);
        }
        auto dia = SelectGroupsDialog::create(m_core.get_current_document(), group.m_uuid, current_source_groups);
        dia->set_transient_for(dynamic_cast<Gtk::Window &>(*get_ancestor(GTK_TYPE_WINDOW)));
        dia->present();
        dia->signal_changed().connect([this, dia, &group] {
            auto groups = dia->get_selected_groups();
            if (groups.size() < 2)
                return;

            decltype(group.m_sources) new_sources;
            for (const auto &uu : groups) {
                auto existing = std::ranges::find_if(group.m_sources, [uu](const auto &x) { return x.group == uu; });
                if (existing == group.m_sources.end()) {
                    new_sources.emplace_back(m_core.get_current_document().get_group(uu).m_active_wrkpl, uu);
                }
                else {
                    new_sources.emplace_back(*existing);
                }
            }
            group.m_sources = new_sources;


            m_core.get_current_document().set_group_update_solid_model_pending(group.m_uuid);
            m_signal_changed.emit(CommitMode::IMMEDIATE);
        });
    }

    GroupLoft &get_group()
    {
        return m_core.get_current_document().get_group<GroupLoft>(m_group_uu);
    }
};

namespace {

class GroupButton : public Gtk::Button, public Changeable {
public:
    GroupButton(const Document &doc, const UUID &current_group) : m_doc(doc), m_current_group(current_group)
    {
        m_label = Gtk::make_managed<Gtk::Label>();
        m_label->set_ellipsize(Pango::EllipsizeMode::END);
        m_label->set_xalign(0);
        set_child(*m_label);
        signal_clicked().connect(sigc::mem_fun(*this, &GroupButton::select_group));
    }

    const auto &get_group() const
    {
        return m_group;
    }

    void set_group(const UUID &group)
    {
        m_group = group;
        update_label();
    }

private:
    const Document &m_doc;
    Gtk::Label *m_label = nullptr;
    UUID m_group;
    const UUID m_current_group;

    void update_label()
    {
        if (m_group)
            m_label->set_text(m_doc.get_group(m_group).m_name);
        else
            m_label->set_text("(None)");
    }

    void select_group()
    {
        auto dia = new SelectGroupDialog(m_doc, m_current_group, m_group);
        dia->set_transient_for(dynamic_cast<Gtk::Window &>(*get_ancestor(GTK_TYPE_WINDOW)));
        dia->present();
        dia->signal_changed().connect([this, dia] {
            auto group = dia->get_selected_group();
            m_group = group;
            update_label();
            m_signal_changed.emit();
        });
    }
};

} // namespace

class GroupEditorSolidModelOperation : public GroupEditorSolidModel {
public:
    GroupEditorSolidModelOperation(Core &core, const UUID &group_uu) : GroupEditorSolidModel(core, group_uu)
    {
        add_operation_combo();

        m_argument_button = Gtk::make_managed<GroupButton>(m_core.get_current_document(), group_uu);
        m_tool_button = Gtk::make_managed<GroupButton>(m_core.get_current_document(), group_uu);

        grid_attach_label_and_widget(*this, "Argument", *m_argument_button, m_top);
        grid_attach_label_and_widget(*this, "Tool", *m_tool_button, m_top);

        auto &group = get_group();

        m_argument_button->set_group(group.m_source_group_argument);
        m_tool_button->set_group(group.m_source_group_tool);

        m_argument_button->signal_changed().connect(sigc::mem_fun(*this, &GroupEditorSolidModelOperation::changed));
        m_tool_button->signal_changed().connect(sigc::mem_fun(*this, &GroupEditorSolidModelOperation::changed));

        auto swap_button = Gtk::make_managed<Gtk::Button>("Swap Argument & Tool");
        attach(*swap_button, 1, m_top++, 1, 1);
        swap_button->signal_clicked().connect([this] {
            auto &group = get_group();
            std::swap(group.m_source_group_argument, group.m_source_group_tool);
            m_argument_button->set_group(group.m_source_group_argument);
            m_tool_button->set_group(group.m_source_group_tool);
            m_core.get_current_document().set_group_update_solid_model_pending(group.m_uuid);
            m_signal_changed.emit(CommitMode::IMMEDIATE);
        });
    }

private:
    GroupButton *m_argument_button = nullptr;
    GroupButton *m_tool_button = nullptr;

    void do_reload() override
    {
        GroupEditorSolidModel::do_reload();
        auto &group = get_group();
        m_argument_button->set_group(group.m_source_group_argument);
        m_tool_button->set_group(group.m_source_group_tool);
    }

    void changed()
    {
        auto &group = get_group();
        group.m_source_group_argument = m_argument_button->get_group();
        group.m_source_group_tool = m_tool_button->get_group();
        m_core.get_current_document().set_group_update_solid_model_pending(group.m_uuid);
        m_signal_changed.emit(CommitMode::IMMEDIATE);
    }

    GroupSolidModelOperation &get_group()
    {
        return m_core.get_current_document().get_group<GroupSolidModelOperation>(m_group_uu);
    }
};

class GroupEditorRevolve : public GroupEditorSweep {
public:
    GroupEditorRevolve(Core &core, const UUID &group_uu) : GroupEditorSweep(core, group_uu)
    {
        auto &group = get_group();
        add_operation_combo();
        {
            auto items = Gtk::StringList::create();
            items->append("Single");
            items->append("Offset");
            items->append("Offset symmetric");

            m_mode_combo = Gtk::make_managed<Gtk::DropDown>(items);
            m_mode_combo->set_selected(static_cast<guint>(group.m_mode));
            m_mode_combo->property_selected().signal_changed().connect([this] {
                if (is_reloading())
                    return;
                auto &group = get_group();
                group.m_mode = static_cast<GroupRevolve::Mode>(m_mode_combo->get_selected());
                m_core.get_current_document().set_group_generate_pending(group.m_uuid);
                m_signal_changed.emit(CommitMode::IMMEDIATE);
            });
            grid_attach_label_and_widget(*this, "Mode", *m_mode_combo, m_top);
        }
    }

    void do_reload() override
    {
        GroupEditorSweep::do_reload();
        auto &group = get_group();
        m_mode_combo->set_selected(static_cast<guint>(group.m_mode));
    }

private:
    GroupRevolve &get_group()
    {
        return m_core.get_current_document().get_group<GroupRevolve>(m_group_uu);
    }

    Gtk::DropDown *m_mode_combo = nullptr;
};
class GroupEditorFillet : public GroupEditor {
public:
    GroupEditorFillet(Core &core, const UUID &group_uu) : GroupEditor(core, group_uu)
    {
        m_radius_sp = Gtk::make_managed<SpinButtonDim>();
        m_radius_sp->set_range(0, 1000);
        auto &group = get_group();
        m_radius_sp->set_value(group.m_radius);
        connect_spinbutton(*m_radius_sp, sigc::mem_fun(*this, &GroupEditorFillet::update_radius));

        grid_attach_label_and_widget(*this, "Radius", *m_radius_sp, m_top);

        {
            auto button = Gtk::make_managed<Gtk::Button>("Select edges…");
            button->signal_clicked().connect([this] { m_signal_trigger_action.emit(ToolID::SELECT_EDGES); });
            attach(*button, 0, m_top++, 2, 1);
        }
    }

    void do_reload() override
    {
        GroupEditor::do_reload();
        auto &group = get_group();
        m_radius_sp->set_value(group.m_radius);
    }

private:
    GroupLocalOperation &get_group()
    {
        return m_core.get_current_document().get_group<GroupLocalOperation>(m_group_uu);
    }

    bool update_radius()
    {
        if (is_reloading())
            return false;
        if (get_group().m_radius == m_radius_sp->get_value())
            return false;
        get_group().m_radius = m_radius_sp->get_value();
        m_core.get_current_document().set_group_update_solid_model_pending(get_group().m_uuid);
        return true;
    }

    SpinButtonDim *m_radius_sp = nullptr;
};

class GroupEditorReference : public GroupEditor {
public:
    GroupEditorReference(Core &core, const UUID &group_uu) : GroupEditor(core, group_uu)
    {
        auto &group = get_group();

        m_switch_xy = Gtk::make_managed<Gtk::Switch>();
        m_switch_xy->set_halign(Gtk::Align::START);
        m_switch_xy->set_valign(Gtk::Align::CENTER);
        grid_attach_label_and_widget(*this, "XY", *m_switch_xy, m_top);
        m_switch_xy->set_active(group.m_show_xy);
        m_switch_xy->property_active().signal_changed().connect([this] {
            if (is_reloading())
                return;
            get_group().m_show_xy = m_switch_xy->get_active();
            get_group().generate(m_core.get_current_document());
            m_signal_changed.emit(CommitMode::IMMEDIATE);
        });

        m_switch_yz = Gtk::make_managed<Gtk::Switch>();
        m_switch_yz->set_halign(Gtk::Align::START);
        m_switch_yz->set_valign(Gtk::Align::CENTER);
        grid_attach_label_and_widget(*this, "YZ", *m_switch_yz, m_top);
        m_switch_yz->set_active(group.m_show_yz);
        m_switch_yz->property_active().signal_changed().connect([this] {
            if (is_reloading())
                return;
            get_group().m_show_yz = m_switch_yz->get_active();
            get_group().generate(m_core.get_current_document());
            m_signal_changed.emit(CommitMode::IMMEDIATE);
        });

        m_switch_zx = Gtk::make_managed<Gtk::Switch>();
        m_switch_zx->set_halign(Gtk::Align::START);
        m_switch_zx->set_valign(Gtk::Align::CENTER);
        grid_attach_label_and_widget(*this, "ZX", *m_switch_zx, m_top);
        m_switch_zx->set_active(group.m_show_zx);
        m_switch_zx->property_active().signal_changed().connect([this] {
            if (is_reloading())
                return;
            get_group().m_show_zx = m_switch_zx->get_active();
            get_group().generate(m_core.get_current_document());
            m_signal_changed.emit(CommitMode::IMMEDIATE);
        });
    }

    void do_reload() override
    {
        GroupEditor::do_reload();
        auto &group = get_group();
        m_switch_xy->set_active(group.m_show_xy);
        m_switch_yz->set_active(group.m_show_yz);
        m_switch_zx->set_active(group.m_show_zx);
    }

private:
    GroupReference &get_group()
    {
        return m_core.get_current_document().get_group<GroupReference>(m_group_uu);
    }

    Gtk::Switch *m_switch_xy = nullptr;
    Gtk::Switch *m_switch_yz = nullptr;
    Gtk::Switch *m_switch_zx = nullptr;
};

class GroupEditorArray : public GroupEditor {
public:
    GroupEditorArray(Core &core, const UUID &group_uu) : GroupEditor(core, group_uu)
    {
        auto &group = get_group();

        m_sp_count = Gtk::make_managed<Gtk::SpinButton>();
        m_sp_count->set_range(1, 100);
        m_sp_count->set_increments(1, 10);
        grid_attach_label_and_widget(*this, "Count", *m_sp_count, m_top);
        m_sp_count->set_value(group.m_count);
        connect_spinbutton(*m_sp_count, sigc::mem_fun(*this, &GroupEditorArray::update_count));


        {
            auto items = Gtk::StringList::create();
            items->append("Original");
            items->append("First copy");
            items->append("Arbitrary");

            m_offset_combo = Gtk::make_managed<Gtk::DropDown>(items);
            m_offset_combo->set_selected(static_cast<guint>(group.m_offset));
            m_offset_combo->property_selected().signal_changed().connect([this] {
                if (is_reloading())
                    return;
                auto &group = get_group();
                group.m_offset = static_cast<GroupLinearArray::Offset>(m_offset_combo->get_selected());
                m_core.get_current_document().set_group_generate_pending(group.m_uuid);
                m_signal_changed.emit(CommitMode::IMMEDIATE);
            });
            grid_attach_label_and_widget(*this, "Offset", *m_offset_combo, m_top);
        }

        {
            auto items = Gtk::StringList::create();
            items->append("Union (acc)");
            items->append("Difference (acc)");
            items->append("Intersection (acc)");
            items->append("From source");

            m_operation_combo = Gtk::make_managed<Gtk::DropDown>(items);
            if (group.m_use_acc)
                m_operation_combo->set_selected(static_cast<guint>(group.get_operation()));
            else
                m_operation_combo->set_selected(3);
            m_operation_combo->property_selected().signal_changed().connect([this] {
                if (is_reloading())
                    return;
                auto &group = get_group();
                const auto sel = m_operation_combo->get_selected();
                if (sel == 3) {
                    group.m_use_acc = false;
                }
                else {
                    group.set_operation(static_cast<GroupExtrude::Operation>(sel));
                    group.m_use_acc = true;
                }
                m_core.get_current_document().set_group_update_solid_model_pending(m_group_uu);
                m_signal_changed.emit(CommitMode::IMMEDIATE);
            });
            grid_attach_label_and_widget(*this, "Operation", *m_operation_combo, m_top);
        }
    }

    void do_reload() override
    {
        GroupEditor::do_reload();
        auto &group = get_group();
        m_sp_count->set_value(group.m_count);
        m_offset_combo->set_selected(static_cast<guint>(group.m_offset));
        if (group.m_use_acc)
            m_operation_combo->set_selected(static_cast<guint>(group.get_operation()));
        else
            m_operation_combo->set_selected(3);
    }

private:
    GroupArray &get_group()
    {
        return m_core.get_current_document().get_group<GroupArray>(m_group_uu);
    }

    bool update_count()
    {
        if (is_reloading())
            return false;
        if (static_cast<int>(get_group().m_count) == m_sp_count->get_value_as_int())
            return false;

        get_group().m_count = m_sp_count->get_value_as_int();
        m_core.get_current_document().set_group_generate_pending(get_group().m_uuid);
        return true;
    }

    Gtk::SpinButton *m_sp_count = nullptr;
    Gtk::DropDown *m_offset_combo = nullptr;
    Gtk::DropDown *m_operation_combo = nullptr;
};

class GroupEditorExplodedCluster : public GroupEditor {
public:
    GroupEditorExplodedCluster(Core &core, const UUID &group_uu) : GroupEditor(core, group_uu)
    {
        {
            auto button = Gtk::make_managed<Gtk::Button>("Unexplode cluster");
            button->signal_clicked().connect([this] { m_signal_trigger_action.emit(ActionID::UNEXPLODE_CLUSTER); });
            attach(*button, 0, m_top++, 2, 1);
        }
    }
};

class GroupEditorClone : public GroupEditor {
public:
    GroupEditorClone(Core &core, const UUID &group_uu) : GroupEditor(core, group_uu)
    {
        auto source_group_name = m_core.get_current_document().get_group(get_group().m_source_group).m_name;
        auto source_label = Gtk::make_managed<Gtk::Label>(source_group_name);
        source_label->set_xalign(0);
        grid_attach_label_and_widget(*this, "Source group", *source_label, m_top);
    }

private:
    GroupClone &get_group()
    {
        return m_core.get_current_document().get_group<GroupClone>(m_group_uu);
    }
};

class GroupEditorPipe : public GroupEditorSweep {
public:
    GroupEditorPipe(Core &core, const UUID &group_uu) : GroupEditorSweep(core, group_uu)
    {
        add_operation_combo();
        m_spine_label = Gtk::make_managed<Gtk::Label>("No entities");
        m_spine_label->set_xalign(0);
        auto button = Gtk::make_managed<Gtk::Button>();
        button->set_child(*m_spine_label);
        button->signal_clicked().connect([this] { m_signal_trigger_action.emit(ToolID::SELECT_SPINE_ENTITIES); });
        grid_attach_label_and_widget(*this, "Spine", *button, m_top);
        update_label();
    }

    void do_reload() override
    {
        GroupEditorSweep::do_reload();
        update_label();
    }

private:
    void update_label()
    {
        auto &group = get_group();
        auto sz = group.m_spine_entities.size();
        std::string label;
        if (sz == 0)
            label = "No entities";
        else
            label = std::format("{} {}", sz, Entity::get_type_name_for_n(EntityType::INVALID, sz));
        m_spine_label->set_label(label);
    }

    GroupPipe &get_group()
    {
        return m_core.get_current_document().get_group<GroupPipe>(m_group_uu);
    }

    Gtk::Label *m_spine_label = nullptr;
};

void GroupEditor::update_name()
{
    auto &group = m_core.get_current_document().get_group(m_group_uu);
    const std::string new_name = m_name_entry->get_text();
    if (group.m_name != new_name) {
        group.m_name = new_name;
        m_signal_changed.emit(CommitMode::IMMEDIATE);
    }
}

void GroupEditor::update_body_name()
{
    auto &group = m_core.get_current_document().get_group(m_group_uu);
    if (!group.m_body)
        return;
    auto &body = *group.m_body;
    const std::string new_name = m_body_entry->get_text();
    if (body.m_name != new_name) {
        body.m_name = new_name;
        m_signal_changed.emit(CommitMode::IMMEDIATE);
    }
}

GroupEditor::GroupEditor(Core &core, const UUID &group_uu) : m_core(core), m_group_uu(group_uu)
{
    set_valign(Gtk::Align::START);
    set_row_spacing(5);
    set_column_spacing(5);
    set_margin(10);
    set_row_homogeneous(true);
    m_type_label = Gtk::manage(new Gtk::Label);
    m_type_label->set_xalign(0);
    auto &group = m_core.get_current_document().get_group(group_uu);
    m_type_label->set_text(group.get_type_name());
    grid_attach_label_and_widget(*this, "Type", *m_type_label, m_top);

    m_name_entry = Gtk::make_managed<Gtk::Entry>();
    m_name_entry->set_hexpand(true);
    m_name_entry->set_text(group.m_name);
    grid_attach_label_and_widget(*this, "Name", *m_name_entry, m_top);
    connect_entry(*m_name_entry, sigc::mem_fun(*this, &GroupEditor::update_name));

    m_body_entry = Gtk::make_managed<Gtk::Entry>();
    if (group.m_body.has_value())
        m_body_entry->set_text(group.m_body->m_name);
    m_body_cb = Gtk::make_managed<Gtk::CheckButton>();
    m_body_cb->set_active(group.m_body.has_value());
    if (&group == core.get_current_document().get_groups_sorted().front())
        m_body_cb->set_sensitive(false);
    m_body_entry->set_sensitive(group.m_body.has_value());

    m_body_color_cb = Gtk::make_managed<Gtk::CheckButton>();
    m_body_color_cb->set_sensitive(group.m_body.has_value());
    m_body_color_cb->set_margin_start(5);
    {
        const bool has_color = group.m_body.has_value() && group.m_body->m_color.has_value();
        m_body_color_cb->set_active(has_color);
        m_body_color_button = Gtk::make_managed<Gtk::ColorDialogButton>();
        {
            auto dia = Gtk::ColorDialog::create();
            dia->set_with_alpha(false);
            m_body_color_button->set_dialog(dia);
        }
        if (has_color) {
            m_body_color_button->set_rgba(rgba_from_color(*group.m_body->m_color));
        }
        else {
            m_body_color_button->set_sensitive(false);
        }
    }
    {
        auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 3);
        box->append(*m_body_cb);
        box->append(*m_body_entry);
        box->append(*m_body_color_cb);
        box->append(*m_body_color_button);
        grid_attach_label_and_widget(*this, "Body", *box, m_top);
    }

    connect_entry(*m_body_entry, sigc::mem_fun(*this, &GroupEditor::update_body_name));

    m_body_cb->signal_toggled().connect([this] {
        if (is_reloading())
            return;
        auto &group = m_core.get_current_document().get_group(m_group_uu);
        if (m_body_cb->get_active())
            group.m_body.emplace();
        else
            group.m_body.reset();
        m_body_entry->set_sensitive(group.m_body.has_value());
        m_body_color_cb->set_sensitive(group.m_body.has_value());
        m_core.get_current_document().set_group_update_solid_model_pending(group.m_uuid);
        m_signal_changed.emit(CommitMode::IMMEDIATE);
    });

    m_body_color_cb->signal_toggled().connect([this] {
        if (is_reloading())
            return;
        auto &group = m_core.get_current_document().get_group(m_group_uu);
        if (!group.m_body)
            return;
        const bool has_color = m_body_color_cb->get_active();
        if (has_color)
            group.m_body->m_color = color_from_rgba(m_body_color_button->get_rgba());
        else
            group.m_body->m_color.reset();
        m_body_color_button->set_sensitive(has_color);

        m_core.get_current_document().set_group_update_solid_model_pending(group.m_uuid);
        m_signal_changed.emit(CommitMode::IMMEDIATE);
    });
    m_body_color_button->property_rgba().signal_changed().connect([this] {
        if (is_reloading())
            return;
        auto &group = m_core.get_current_document().get_group(m_group_uu);
        if (!group.m_body)
            return;
        if (!group.m_body->m_color)
            return;
        group.m_body->m_color = color_from_rgba(m_body_color_button->get_rgba());

        m_core.get_current_document().set_group_update_solid_model_pending(group.m_uuid);
        m_signal_changed.emit(CommitMode::IMMEDIATE);
    });
}

GroupEditor *GroupEditor::create(Core &core, const UUID &group_uu)
{
    auto &group = core.get_current_document().get_group(group_uu);
    switch (group.get_type()) {
    case Group::Type::SKETCH:
        return Gtk::make_managed<GroupEditorSketch>(core, group_uu);
    case Group::Type::EXTRUDE:
        return Gtk::make_managed<GroupEditorExtrude>(core, group_uu);
    case Group::Type::LATHE:
        return Gtk::make_managed<GroupEditorLathe>(core, group_uu);
    case Group::Type::REVOLVE:
        return Gtk::make_managed<GroupEditorRevolve>(core, group_uu);
    case Group::Type::FILLET:
    case Group::Type::CHAMFER:
        return Gtk::make_managed<GroupEditorFillet>(core, group_uu);
    case Group::Type::REFERENCE:
        return Gtk::make_managed<GroupEditorReference>(core, group_uu);
    case Group::Type::LINEAR_ARRAY:
    case Group::Type::POLAR_ARRAY:
        return Gtk::make_managed<GroupEditorArray>(core, group_uu);
    case Group::Type::LOFT:
        return Gtk::make_managed<GroupEditorLoft>(core, group_uu);
    case Group::Type::MIRROR_VERTICAL:
    case Group::Type::MIRROR_HORIZONTAL:
        return Gtk::make_managed<GroupEditorMirror>(core, group_uu);
    case Group::Type::EXPLODED_CLUSTER:
        return Gtk::make_managed<GroupEditorExplodedCluster>(core, group_uu);
    case Group::Type::SOLID_MODEL_OPERATION:
        return Gtk::make_managed<GroupEditorSolidModelOperation>(core, group_uu);
    case Group::Type::CLONE:
        return Gtk::make_managed<GroupEditorClone>(core, group_uu);
    case Group::Type::PIPE:
        return Gtk::make_managed<GroupEditorPipe>(core, group_uu);
    default:
        return Gtk::make_managed<GroupEditor>(core, group_uu);
    }
}

void GroupEditor::do_reload()
{
    auto &group = m_core.get_current_document().get_group(m_group_uu);
    m_name_entry->set_text(group.m_name);
    if (group.m_body.has_value()) {
        m_body_entry->set_text(group.m_body->m_name);
        m_body_color_cb->set_active(group.m_body->m_color.has_value());
        if (group.m_body->m_color.has_value())
            m_body_color_button->set_rgba(rgba_from_color(*group.m_body->m_color));
        m_body_color_button->set_sensitive(group.m_body->m_color.has_value());
    }
    m_body_cb->set_active(group.m_body.has_value());
    m_body_color_cb->set_active(group.m_body.has_value() && group.m_body->m_color.has_value());
}

void GroupEditor::reload()
{
    m_reloading = true;
    do_reload();
    m_reloading = false;
}

GroupEditor::~GroupEditor()
{
}

} // namespace dune3d
