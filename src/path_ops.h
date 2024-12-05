#pragma once

#include <sstream>

#include "config.h"

template <typename char_type>
inline auto operator + (const char_type *lhs, const zz::fs::path &rhs)
{
    std::basic_ostringstream<char_type> ss;
    ss << lhs << rhs.native();
    return ss.str();
}

template <typename char_type>
inline auto operator + (const zz::fs::path &lhs, const char_type *rhs)
{
    std::basic_ostringstream<zz::fs::path::value_type> ss;
    ss << lhs.native() << rhs;
    return ss.str();
}
