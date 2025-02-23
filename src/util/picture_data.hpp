#pragma once
#include <stdint.h>
#include <vector>
#include "util/uuid.hpp"

namespace dune3d {
class PictureData {
public:
    PictureData(const UUID &uu, unsigned int w, unsigned int h, std::vector<uint32_t> &&d);
    const UUID m_uuid;
    const unsigned int m_width;
    const unsigned int m_height;
    const std::vector<uint32_t> m_data;
};
} // namespace dune3d
