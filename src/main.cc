#include <fstream>
#include <iostream>
#include <stdexcept>
#include <type_traits>

#include <boost/program_options.hpp>

#include "path_ops.h"

#include "options.h"
#include "version.h"
#include "pdf2zip.h"
#include "rar2zip.h"
#include "zip2zip.h"

using namespace zz;
using namespace std;
namespace po = boost::program_options;

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
        ? "*.pdf|*.rar|*.zip"
        : "*.pdf|*.zip";
    po::options_description desc(
        "Usage: " + fs::path(argv[0]).stem().string() + " [options] <" + patterns + ">...\n"
        "\n"
        "Options");
    desc.add_options()
        ("help,h", "print this help")
        ("quiet,q", "quiet (no output)")
        ("version,v", "print the version");

    options opts;
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
