#include "about_dialog.hpp"
#include "util/version.hpp"

namespace dune3d {

AboutDialog::AboutDialog() : Gtk::AboutDialog()
{
    std::string version = Version::get_string() + " " + Version::name;
    if (strlen(Version::commit)) {
        version += "\nCommit " + std::string(Version::commit);
    }
    set_version(version);
    set_program_name("Dune 3D");
    std::vector<Glib::ustring> authors;
    authors.push_back("Lukas K. <lukas@dune3d.org>");
    set_authors(authors);
    set_license_type(Gtk::License::GPL_3_0);
    set_copyright("Copyright © 2024 Lukas K., et al.");
    set_website("https://dune3d.org/");
    set_website_label("dune3d.org");
    set_comments("a parametric 3D CAD");

    set_logo_icon_name("dune3d");
}

} // namespace dune3d
