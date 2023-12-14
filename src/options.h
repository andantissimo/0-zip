#pragma once

#include <string>
#include <vector>

#include "config.h"

namespace zz
{
    struct options
    {
        bool                                quiet    = false;
        std::pair<std::string, std::string> charsets = { "cp932", "utf8" };
        std::vector<fs::path::string_type>  excludes = {};
        bool                                rename   = false;
    };
}
