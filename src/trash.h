#pragma once

#include "config.h"

#if __has_include(<filesystem>) && !__APPLE__
namespace std::filesystem
#elif __has_include(<experimental/filesystem>) && !__APPLE__
namespace std::experimental::filesystem
#else
namespace boost::filesystem
#endif
{
    bool trash(const path &);
    bool trash(const path &, zz::ec::error_code &) noexcept;
}
