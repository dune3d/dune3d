#include "log_view.hpp"
#include <iomanip>
#include "util/gtk_util.hpp"

namespace dune3d {

static Gtk::TreeViewColumn *tree_view_append_column_ellipsis(Gtk::TreeView *view, const std::string &name,
                                                             const Gtk::TreeModelColumnBase &column,
                                                             Pango::EllipsizeMode ellipsize)
{
    auto cr = Gtk::manage(new Gtk::CellRendererText());
    auto tvc = Gtk::manage(new Gtk::TreeViewColumn(name, *cr));
    tvc->add_attribute(cr->property_text(), column);
    cr->property_ellipsize() = ellipsize;
    view->append_column(*tvc);
    return tvc;
}


LogView::LogView() : Gtk::Box(Gtk::Orientation::VERTICAL, 0)
{
    bbox = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 8));
    bbox->set_margin_bottom(8);
    bbox->set_margin_top(8);
    bbox->set_margin_start(8);
    bbox->set_margin_end(8);

    auto clear_button = Gtk::make_managed<Gtk::Button>("Clear");
    clear_button->signal_clicked().connect(sigc::mem_fun(*this, &LogView::clear));
    bbox->append(*clear_button);

    {
        auto lbox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 0);
        lbox->get_style_context()->add_class("linked");

        auto add_level_button = [this, lbox](Logger::Level l) {
            auto bu = Gtk::manage(new Gtk::ToggleButton(Logger::level_to_string(l)));
            bu->signal_toggled().connect([this, bu, l] {
                if (bu->get_active()) {
                    levels_visible.insert(l);
                }
                else {
                    if (levels_visible.count(l))
                        levels_visible.erase(l);
                }
                if (store_filtered)
                    store_filtered->refilter();
            });

            lbox->append(*bu);
            return bu;
        };

        add_level_button(Logger::Level::CRITICAL)->set_active(true);
        add_level_button(Logger::Level::WARNING)->set_active(true);
        add_level_button(Logger::Level::INFO);
        add_level_button(Logger::Level::DEBUG);

        levels_visible = {Logger::Level::CRITICAL, Logger::Level::WARNING};

        bbox->append(*lbox);
    }

    auto follow_cb = Gtk::make_managed<Gtk::CheckButton>("Follow");
    follow_cb->set_active(true);
    follow_cb->signal_toggled().connect([this, follow_cb] {
        follow = follow_cb->get_active();
        if (follow) {
            auto it = store->children().end();
            if (it) {
                it--;
                tree_view->scroll_to_row(store->get_path(it));
            }
        }
    });
    bbox->append(*follow_cb);

    auto copy_button = Gtk::make_managed<Gtk::Button>();
    copy_button->set_halign(Gtk::Align::END);
    copy_button->set_hexpand(true);
    copy_button->set_tooltip_text("Copy to clipboard");
    copy_button->set_image_from_icon_name("edit-copy-symbolic");
    copy_button->signal_clicked().connect(sigc::mem_fun(*this, &LogView::copy_to_clipboard));
    bbox->append(*copy_button);

    append(*bbox);

    {
        auto sep = Gtk::make_managed<Gtk::Separator>(Gtk::Orientation::HORIZONTAL);
        append(*sep);
    }

    sc = Gtk::make_managed<Gtk::ScrolledWindow>();
    sc->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    sc->set_vexpand(true);

    store = Gtk::ListStore::create(list_columns);
    store_filtered = Gtk::TreeModelFilter::create(store);
    store_filtered->set_visible_func([this](const Gtk::TreeModel::const_iterator &it) -> bool {
        const Gtk::TreeModel::ConstRow row = *it;
        auto level = row[list_columns.level];
        return levels_visible.count(level);
    });


    tree_view = Gtk::manage(new Gtk::TreeView(store_filtered));
    tree_view->append_column("Seq", list_columns.seq);

    {
        auto cr = Gtk::manage(new Gtk::CellRendererText());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Level", *cr));
        tvc->set_cell_data_func(*cr, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::const_iterator &it) {
            Gtk::TreeModel::ConstRow row = *it;
            auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
            mcr->property_text() = Logger::level_to_string(row[list_columns.level]);
        });
        tree_view->append_column(*tvc);
    }
    {
        auto cr = Gtk::manage(new Gtk::CellRendererText());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Domain", *cr));
        tvc->set_cell_data_func(*cr, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::const_iterator &it) {
            Gtk::TreeModel::ConstRow row = *it;
            auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
            mcr->property_text() = Logger::domain_to_string(row[list_columns.domain]);
        });
        tree_view->append_column(*tvc);
    }
    tree_view->append_column("Message", list_columns.message);
    tree_view_append_column_ellipsis(tree_view, "Detail", list_columns.detail, Pango::EllipsizeMode::END);

    sc->set_child(*tree_view);
    append(*sc);
}

void LogView::push_log(const Logger::Item &it)
{
    auto item = store->append();
    Gtk::TreeModel::Row row = *item;
    row[list_columns.seq] = it.seq;
    row[list_columns.domain] = it.domain;
    row[list_columns.message] = it.message;
    row[list_columns.detail] = it.detail;
    row[list_columns.level] = it.level;

    if (follow && tree_view->get_realized()) {
        tree_view->scroll_to_row(store->get_path(item));
    }
    s_signal_logged.emit(it);
}

void LogView::copy_to_clipboard()
{
    std::ostringstream oss;
    static constexpr auto w_seq = 4;
    static constexpr auto w_level = 8;
    static constexpr auto w_domain = 24;
    oss << std::left;
    oss << std::setw(w_seq) << "Seq"
        << "| ";
    oss << std::setw(w_level) << "Level"
        << "| ";
    oss << std::setw(w_domain) << "Domain"
        << "| Message; Detail";
    oss << std::endl;
    for (const auto &row : store_filtered->children()) {
        oss << std::setw(w_seq) << row[list_columns.seq] << "| ";
        oss << std::setw(w_level) << Logger::level_to_string(row[list_columns.level]) << "| ";
        oss << std::setw(w_domain) << Logger::domain_to_string(row[list_columns.domain]) << "| ";
        oss << row[list_columns.message] << "; ";
        oss << row[list_columns.detail];
        oss << std::endl;
    }

    Gdk::Display::get_default()->get_clipboard()->set_text(oss.str());
}

void LogView::clear()
{
    store->clear();
}

} // namespace dune3d
