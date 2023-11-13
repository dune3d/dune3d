#pragma once
#include "nlohmann/json_fwd.hpp"
#include "util/uuid.hpp"
#include <span>
#include <filesystem>
#include <algorithm>

namespace dune3d {
using json = nlohmann::json;
json load_json_from_file(const std::filesystem::path &filename);
void save_json_to_file(const std::filesystem::path &filename, const json &j);
json json_from_resource(const std::string &rsrc);

std::filesystem::path get_config_dir();
void create_config_dir();

template <typename Map, typename F> static void map_erase_if(Map &m, F pred)
{
    for (typename Map::iterator i = m.begin(); (i = std::find_if(i, m.end(), pred)) != m.end(); m.erase(i++))
        ;
}

UUID hash_uuids(const UUID &ns, const std::vector<UUID> &uuids, std::span<const uint8_t> extra_data = {});

std::string append_suffix_if_required(const std::string &s, const std::string &suffix);

} // namespace dune3d
