#pragma once
#include <gtkmm.h>
#include "preferences/preferences.hpp"

namespace dune3d {

class PreferencesRow : public Gtk::Box {
public:
    PreferencesRow(const std::string &title, const std::string &subtitle, Preferences &prefs);
    virtual void activate()
    {
    }

protected:
    Preferences &preferences;
    void set_title(const std::string &t);
    void set_subtitle(const std::string &t);

private:
    Gtk::Label *label_title = nullptr;
    Gtk::Label *label_subtitle = nullptr;
};


class PreferencesRowBool : public PreferencesRow {
public:
    PreferencesRowBool(const std::string &title, const std::string &subtitle, Preferences &prefs, bool &v);
    void activate() override;

private:
    Gtk::Switch *sw = nullptr;
};

class PreferencesRowBoolButton : public PreferencesRow {
public:
    PreferencesRowBoolButton(const std::string &title, const std::string &subtitle, const std::string &label_true,
                             const std::string &label_false, Preferences &prefs, bool &v);
};

template <typename T> class PreferencesRowNumeric : public PreferencesRow {
public:
    PreferencesRowNumeric(const std::string &title, const std::string &subtitle, Preferences &prefs, T &v)
        : PreferencesRow(title, subtitle, prefs), value(v)
    {
        sp = Gtk::manage(new Gtk::SpinButton);
        sp->set_valign(Gtk::Align::CENTER);
        append(*sp);
    }

    Gtk::SpinButton &get_spinbutton()
    {
        return *sp;
    }

    void bind()
    {
        sp->set_value(value);
        sp->signal_value_changed().connect([this] {
            value = sp->get_value();
            preferences.signal_changed().emit();
        });
    }

private:
    T &value;
    Gtk::SpinButton *sp = nullptr;
};

template <typename T> class PreferencesRowEnum : public PreferencesRow {
public:
    PreferencesRowEnum(const std::string &title, const std::string &subtitle, Preferences &prefs, T &v,
                       const std::vector<std::pair<T, std::string>> &names)
        : PreferencesRow(title, subtitle, prefs), value(v)
    {
        combo = Gtk::manage(new Gtk::ComboBoxText);
        for (const auto &[key, enum_value] : names) {
            combo->append(std::to_string(static_cast<int>(key)), enum_value);
        }
        combo->set_valign(Gtk::Align::CENTER);
        append(*combo);
    }

    void bind()
    {
        combo->set_active_id(std::to_string(static_cast<int>(value)));
        combo->signal_changed().connect([this] {
            value = static_cast<T>(std::stoi(combo->get_active_id()));
            preferences.signal_changed().emit();
        });
    }

private:
    T &value;
    Gtk::ComboBoxText *combo = nullptr;
};

class PreferencesGroup : public Gtk::Box {
public:
    PreferencesGroup(const std::string &title);
    void add_row(PreferencesRow &row);

    void set_placeholder(Gtk::Widget &w);

private:
    Gtk::ListBox *m_listbox = nullptr;
};

} // namespace dune3d
