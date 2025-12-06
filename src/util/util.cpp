#include "util.hpp"
#include "fs_util.hpp"
#include "nlohmann/json.hpp"
#include <fstream>
#include <giomm.h>
#ifdef G_OS_WIN32
#include <windows.h>
#include <shlobj.h>
#include <iostream>
#endif

namespace dune3d {
json load_json_from_file(const std::filesystem::path &filename)
{
    json j;
    std::ifstream ifs{filename};
    if (!ifs.is_open()) {
        throw std::runtime_error("file " + filename.string() + " not opened");
    }
    ifs >> j;
    ifs.close();
    return j;
}

void save_json_to_file(const std::filesystem::path &filename, const json &j)
{
    Glib::file_set_contents(filename.string(), j.dump(4));
}

json json_from_resource(const std::string &rsrc)
{
    auto json_bytes = Gio::Resource::lookup_data_global(rsrc);
    gsize size = json_bytes->get_size();
    return json::parse((const char *)json_bytes->get_data(size));
}


std::filesystem::path get_config_dir()
{
    return path_from_string(Glib::get_user_config_dir()) / "dune3d";
}

void create_config_dir()
{
    auto config_dir = path_to_string(get_config_dir());
    if (!Glib::file_test(config_dir, Glib::FileTest::EXISTS))
        Gio::File::create_for_path(config_dir)->make_directory_with_parents();
}

UUID hash_uuids(const UUID &ns, const std::vector<UUID> &uuids, std::span<const uint8_t> extra_data)
{
    std::vector<unsigned char> path_bytes(uuids.size() * UUID::size + extra_data.size());
    auto p = path_bytes.data();
    for (const auto &uu : uuids) {
        for (size_t i = 0; i < UUID::size; i++) {
            *p++ = uu.get_bytes()[i];
        }
    }
    for (size_t i = 0; i < extra_data.size(); i++) {
        *p++ = extra_data[i];
    }
    return UUID::UUID5(ns, path_bytes.data(), path_bytes.size());
}

std::string append_suffix_if_required(const std::string &s, const std::string &suffix)
{
    if (s.ends_with(suffix))
        return s;
    else
        return s + suffix;
}


static std::locale setup_locale()
{
    std::locale::global(std::locale::classic());
    char decimal_sep = '.';
#ifdef G_OS_WIN32
    TCHAR szSep[8];
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, szSep, 8);
    if (szSep[0] == ',') {
        decimal_sep = ',';
    }
#else
    try {
        // Try setting the "native locale", in order to retain
        // things like decimal separator.  This might fail in
        // case the user has no notion of a native locale.
        auto user_locale = std::locale("");
        decimal_sep = std::use_facet<std::numpunct<char>>(user_locale).decimal_point();
    }
    catch (...) {
        // just proceed
    }
#endif

    class comma : public std::numpunct<char> {
    public:
        comma(char c) : dp(c)
        {
        }
        char do_decimal_point() const override
        {
            return dp;
        } // comma

    private:
        const char dp;
    };

    return std::locale(std::locale::classic(), new comma(decimal_sep));
}

const std::locale &get_locale()
{
    static std::locale the_locale = setup_locale();
    return the_locale;
}

void add_to_recent_docs(const std::filesystem::path &path)
{
#ifdef G_OS_WIN32
    SHAddToRecentDocs(SHARD_PATHW, path.c_str());
#endif
}

} // namespace dune3d
