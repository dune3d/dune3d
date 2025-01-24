#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "document/document.hpp"
#include "document/entity/entity_text.hpp"
#include "document/group/group.hpp"
#include "document/group/igroup_solid_model.hpp"
#include "document/solid_model.hpp"
#include "preferences/preferences.hpp"
#include "util/text_render.hpp"
#include "util/fs_util.hpp"
#include <glibmm.h>
#include <pangomm/init.h>

namespace py = pybind11;

using namespace dune3d;

PYBIND11_MODULE(dune3d_py, m)
{
    Glib::init();
    Pango::init();

    py::class_<SolidModel>(m, "SolidModel").def("export_stl", [](SolidModel &solid_model, const std::string &path) {
        solid_model.export_stl(path_from_string(path));
    });

    py::class_<Group>(m, "Group")
            .def_readonly("name", &Group::m_name)
            .def_property_readonly(
                    "solid_model",
                    [](Group &group) -> const SolidModel * {
                        if (auto gr_solid = dynamic_cast<const IGroupSolidModel *>(&group))
                            return gr_solid->get_solid_model();
                        return nullptr;
                    },
                    py::return_value_policy::reference);

    py::class_<Document>(m, "Document")
            .def_static("new_from_file",
                        [](const std::string &path) { return Document::new_from_file(path_from_string(path)); })
            .def("get_groups_sorted", static_cast<std::vector<Group *> (Document::*)()>(&Document::get_groups_sorted),
                 py::return_value_policy::reference)
            .def("render_texts", [](Document &doc) {
                for (auto &[uu, it] : doc.m_entities) {
                    if (auto en = dynamic_cast<EntityText *>(it.get())) {
                        PangoFontMap *font_map = pango_cairo_font_map_get_default();
                        PangoContext *ctx = pango_font_map_create_context(font_map);
                        render_text(*en, Glib::wrap(ctx), doc);
                        doc.set_group_generate_pending(en->m_group);
                    }
                }
                doc.update_pending();
            });
}

const Preferences &Preferences::get()
{
    static Preferences the_preferences;
    return the_preferences;
}
