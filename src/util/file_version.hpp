#pragma once
#include "nlohmann/json_fwd.hpp"

namespace dune3d {
using json = nlohmann::json;

class FileVersion {
public:
    FileVersion(unsigned int a);
    FileVersion(unsigned int a, unsigned int f);
    FileVersion(unsigned int a, const json &j);

    unsigned int get_app() const
    {
        return m_app;
    }

    unsigned int get_file() const
    {
        return m_file;
    }

    void update_file_from_app();

    void serialize(json &j) const;

    std::string get_message() const;

    static const std::string learn_more_markup;

private:
    unsigned int m_app = 0;
    unsigned int m_file = 0;
};

} // namespace dune3d
