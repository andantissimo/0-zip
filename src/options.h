#pragma once

#include <string>

namespace zz
{
    struct options
    {
        bool                                quiet   = false;
        std::pair<std::string, std::string> charset = { "cp932", "utf8" };
        bool                                rename  = false;
    };
}
