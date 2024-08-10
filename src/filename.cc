#include <iomanip>
#include <sstream>

#include "filename.h"

using namespace std;

using char_type   = zz::fs::path::value_type;
using string_type = zz::fs::path::string_type;

#if defined(_WIN32) || defined(BOOST_WINDOWS_API)
#define T(s) (L ## s)
#else
#define T(s) (s)
#endif

static auto canonicalize(basic_string_view<char_type> extension)
{
    string_type s(extension);
    if (s.size() <= 2)
        return s;
    if (s == T(".jfi") || s == T(".jfif") || s == T(".jif") || s == T(".jpe") || s == T(".jpeg"))
        return T(".jpg"s);
    if (s == T(".tif"))
        return T(".tiff"s);
    return s;
}

string_type make_filename(size_t number, basic_string_view<char_type> extension)
{
    basic_ostringstream<char_type> ss;
    ss << setw(3) << setfill<char_type>('0') << number << canonicalize(extension);
    return ss.str();
}
