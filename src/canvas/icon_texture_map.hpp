#pragma once
#include <map>

namespace dune3d::IconTexture {
enum class IconTextureID;
struct Position {
    unsigned int x;
    unsigned int y;
};
extern const std::map<IconTextureID, Position> icon_texture_map;
extern const unsigned int icon_size;
extern const unsigned int icon_border;


} // namespace dune3d::IconTexture
