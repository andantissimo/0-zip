#include <fstream>
#include <iostream>
#include <stdexcept>
#include <type_traits>

#include <boost/program_options.hpp>

#include "path_ops.h"

#include "options.h"
#include "version.h"
#include "dir2zip.h"
#include "pdf2zip.h"
#include "rar2zip.h"
#include "zip2zip.h"

#ifdef _UNICODE
#define tvalue wvalue
#else
#define tvalue value
#endif

using namespace zz;
using namespace std;
namespace po = boost::program_options;

namespace std
{
    static inline auto & operator >> (istream &in, pair<string, string> &value)
    {
        string s;
        in >> s;
        const auto i = s.find(',');
        value.first  = i == string::npos ? s : s.substr(0, i);
        value.second = i == string::npos ? s : s.substr(i + 1);
        return in;
    }
    static inline auto & operator << (ostream &out, const pair<string, string> &value)
    {
        return out << value.first << ',' << value.second;
    }
}

#ifdef _UNICODE
int wmain(int argc, wchar_t *argv[])
#else
int main(int argc, char *argv[])
#endif
#ifdef NDEBUG
try
#endif
{
    using char_type   = remove_pointer_t<remove_pointer_t<decltype(argv)>>;
    using string_type = basic_string<char_type>;

    const auto patterns = zz::rar_exists()
        ? "directory|*.pdf|*.rar|*.zip"
        : "directory|*.pdf|*.zip";
    po::options_description desc(
        "Usage: " + fs::path(argv[0]).stem().string() + " [options] <" + patterns + ">...\n"
        "\n"
        "Options");
    options opts;
    desc.add_options()
        ("help,h"   , "print this help")
        ("quiet,q"  , "quiet mode")
        ("version,v", "print the version")
        ("charset,O", po::value(&opts.charsets)->value_name("IN,OUT")->default_value(opts.charsets),
                      "specify character encodings")
        ("exclude,x", po::tvalue(&opts.excludes)->value_name("PATTERN"),
                      "exclude files with the given patterns")
        ("rename,n" , "rename entries to sequential numbers");
    vector<string_type> args;
    try {
        auto parsed = po::parse_command_line(argc, argv, desc);
        po::variables_map vmap;
        po::store(parsed, vmap);
        po::notify(vmap);
        args = po::collect_unrecognized(parsed.options, po::include_positional);
        if (vmap.count("version")) {
            cerr << version << endl;
            exit(0);
        }
        if (vmap.count("help") || args.empty()) {
            cerr << desc << endl;
            exit(0);
        }
        if (vmap.count("quiet"))
            opts.quiet = true;
        if (vmap.count("rename"))
            opts.rename = true;
    } catch (...) {
        cerr << desc << endl;
        exit(2);
    }

    for (auto it = begin(args); it != end(args); ++it) {
        auto path = fs::path(*it);
        if (!fs::exists(path))
            throw runtime_error("file not found: " + path.filename());

        auto i = distance(begin(args), it);
        if (!opts.quiet)
            cout << (1 + i) << ". " << path.filename() << endl;

        if (fs::is_directory(path)) {
            dir2zip(path, opts);
            continue;
        }
        uint32_t signature = 0;
        {
            io::ifstream file;
            file.open(path, ios::binary);
            file.read(reinterpret_cast<char *>(&signature), sizeof signature);
        }
        switch (signature) {
        case pdf_signature:
            pdf2zip(path, opts);
            break;
        case rar_signature:
            rar2zip(path, opts);
            break;
        case zip_signature:
            zip2zip(path, opts);
            break;
        }
    }

    return 0;
}
#ifdef NDEBUG
catch (const std::exception &ex)
{
    cerr << "Error: " << ex.what() << endl;
    return 1;
}
#endif
