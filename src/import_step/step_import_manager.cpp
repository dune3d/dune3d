#include "step_import_manager.hpp"
#include "import.hpp"
#include "nlohmann/json.hpp"
#include "util/util.hpp"
#include <glibmm.h>
#include <giomm.h>
#include "util/fs_util.hpp"

namespace dune3d {

using json = nlohmann::json;
namespace fs = std::filesystem;

static std::filesystem::path get_cache_dir()
{
    return std::filesystem::path(Glib::get_user_cache_dir()) / "dune3d" / "step";
}
static void create_cache_dir()
{
    auto cache_dir = get_cache_dir();
    if (!Glib::file_test(path_to_string(cache_dir), Glib::FileTest::EXISTS))
        Gio::File::create_for_path(path_to_string(cache_dir))->make_directory_with_parents();
}

STEPImportManager::STEPImportManager()
{
    create_cache_dir();
}

STEPImportManager &STEPImportManager::get()
{
    static STEPImportManager instance;
    return instance;
}

static auto hash_file(const std::filesystem::path &path)
{
    auto mapped_a = g_mapped_file_new(path_to_string(path).c_str(), false, NULL);
    if (!mapped_a) {
        return std::string{};
    }

    Glib::Checksum chk(Glib::Checksum::Type::SHA256);
    chk.update(reinterpret_cast<const unsigned char *>(g_mapped_file_get_contents(mapped_a)),
               g_mapped_file_get_length(mapped_a));
    auto dig = chk.get_string();

    g_mapped_file_unref(mapped_a);

    return dig;
}

namespace face {
template <typename T> void to_json(json &j, const TVertex<T> &r)
{
    j = json::array();
    j.push_back(r.x);
    j.push_back(r.y);
    j.push_back(r.z);
}


template <typename T> void from_json(const json &j, TVertex<T> &r)
{
    j.at(0).get_to(r.x);
    j.at(1).get_to(r.y);
    j.at(2).get_to(r.z);
}


void to_json(json &j, const Color &c)
{
    j = json::array();
    j.push_back(c.r);
    j.push_back(c.g);
    j.push_back(c.b);
}

void from_json(const json &j, Color &c)
{
    j.at(0).get_to(c.r);
    j.at(1).get_to(c.g);
    j.at(2).get_to(c.b);
}

void to_json(json &j, const Face &f)
{
    j = {{"color", f.color},
         {"vertices", f.vertices},
         {"normals", f.normals},
         {"triangle_indices", f.triangle_indices}};
}

void from_json(const json &j, Face &f)
{
    j.at("color").get_to(f.color);
    j.at("vertices").get_to(f.vertices);
    j.at("normals").get_to(f.normals);
    j.at("triangle_indices").get_to(f.triangle_indices);
}

} // namespace face

namespace STEPImporter {

static void to_json(json &j, const Result &r)
{
    j = {{"faces", r.faces}, {"points", r.points}};
}

static void from_json(const json &j, Result &r)
{

    j.at("points").get_to(r.points);
    j.at("faces").get_to(r.faces);
}

} // namespace STEPImporter


std::shared_ptr<ImportedSTEP> STEPImportManager::import_step(const std::filesystem::path &path)
{
    if (m_imported.contains(path))
        return m_imported.at(path);

    auto imported = std::make_shared<ImportedSTEP>(path);
    imported->ready = true;

    auto hash = hash_file(path);
    auto cache_path = get_cache_dir() / (hash + ".ubjson");

    if (fs::exists(cache_path)) {
        // json::from_ubjson(
        auto rd = Glib::file_get_contents(cache_path.string());
        auto j = json::from_ubjson(std::span(rd.data(), rd.size()));
        j.get_to(imported->result);
    }
    else {
        imported->result = STEPImporter::import(path.generic_string());

        json j = imported->result;
        auto bs = json::to_ubjson(j);
        Glib::file_set_contents(cache_path.string(), reinterpret_cast<const gchar *>(bs.data()), bs.size());
        // save_json_to_file(cache_path, j);
    }
    m_imported.emplace(path, imported);
    return imported;
}


} // namespace dune3d
