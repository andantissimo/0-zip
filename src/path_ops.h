#pragma once

#include <sstream>

#include "config.h"

template <typename char_type>
inline auto operator + (const char_type *lhs, const az::fs::path &rhs)
{
    std::basic_ostringstream<char_type> ss;
    ss << lhs << rhs;
    return ss.str();
}
