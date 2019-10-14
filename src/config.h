#pragma once

#if __has_include(<filesystem>) && !__APPLE__
#include <filesystem>
namespace zz
{
    namespace fs = std::filesystem;
    namespace io = std;
    namespace ec = std;
}
#elif __has_include(<experimental/filesystem>) && !__APPLE__
#include <experimental/filesystem>
namespace zz
{
    namespace fs = std::experimental::filesystem;
    namespace io = std;
    namespace ec = std;
}
#else
#include <boost/filesystem.hpp>
namespace zz
{
    namespace fs = boost::filesystem;
    namespace io = boost::filesystem;
    namespace ec = boost::system;
}
#endif
