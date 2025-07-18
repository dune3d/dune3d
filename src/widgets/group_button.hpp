#include <gtkmm.h>
#include "util/changeable.hpp"
#include "util/uuid.hpp"

namespace dune3d {

class Document;

class GroupButton : public Gtk::Button, public Changeable {
public:
    GroupButton(const Document &doc, const UUID &current_group);

    const UUID &get_group() const;
    void set_group(const UUID &group);

private:
    const Document &m_doc;
    Gtk::Label *m_label = nullptr;
    UUID m_group;
    const UUID m_current_group;

    void update_label();
    void select_group();
};

} // namespace dune3d
