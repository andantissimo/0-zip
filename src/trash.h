#pragma once

#include "config.h"

#if __has_include(<filesystem>) && !NO_STD_FILESYSTEM
namespace std::filesystem
#elif __has_include(<experimental/filesystem>) && !NO_STD_FILESYSTEM
namespace std::experimental::filesystem
#else
namespace boost::filesystem
#endif
{
    bool trash(const path &);
    bool trash(const path &, az::ec::error_code &) noexcept;
}
