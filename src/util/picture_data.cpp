#include "picture_data.hpp"

namespace dune3d {
PictureData::PictureData(const UUID &uu, unsigned int w, unsigned int h, std::vector<uint32_t> &&d)
    : m_uuid(uu), m_width(w), m_height(h), m_data(std::move(d))
{
}
} // namespace dune3d
