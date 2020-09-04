#pragma once

#if __has_include(<filesystem>)
#include <filesystem>
namespace zz
{
    namespace fs
    {
        using std::filesystem::filesystem_error;
        using std::filesystem::path;
        using std::filesystem::absolute;
        using std::filesystem::exists;
        using std::filesystem::file_size;
        using std::filesystem::last_write_time;
        using std::filesystem::rename;
    }
    namespace io
    {
        using std::ifstream;
        using std::ofstream;
    }
    namespace ec
    {
        using std::error_code;
        using std::system_category;
    }
}
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace zz
{
    namespace fs
    {
        using std::experimental::filesystem::filesystem_error;
        using std::experimental::filesystem::path;
        using std::experimental::filesystem::absolute;
        using std::experimental::filesystem::exists;
        using std::experimental::filesystem::file_size;
        using std::experimental::filesystem::last_write_time;
        using std::experimental::filesystem::rename;
    }
    namespace io
    {
        using std::ifstream;
        using std::ofstream;
    }
    namespace ec
    {
        using std::error_code;
        using std::system_category;
    }
}
#else
#include <boost/filesystem.hpp>
namespace zz
{
    namespace fs
    {
        using boost::filesystem::filesystem_error;
        using boost::filesystem::path;
        using boost::filesystem::absolute;
        using boost::filesystem::exists;
        using boost::filesystem::file_size;
        using boost::filesystem::last_write_time;
        using boost::filesystem::rename;
    }
    namespace io
    {
        using boost::filesystem::ifstream;
        using boost::filesystem::ofstream;
    }
    namespace ec
    {
        using boost::system::error_code;
        using boost::system::system_category;
    }
}
#endif
