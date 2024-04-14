#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244 4245)
#endif
#include <boost/crc.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "dostime.h"
#include "path_ops.h"
#include "pkzip_io.h"
#include "strnatcmp.h"

#include "dir2zip.h"

using namespace zz;
using namespace std;

using crc32_t = boost::crc_32_type;

template <typename output_iterator_type>
static void enumerate_files(const fs::path &path, output_iterator_type files)
{
    for (const auto &entry : fs::directory_iterator(path)) {
        if (entry.is_directory())
            enumerate_files(entry.path(), files);
        else if (entry.is_regular_file())
            *files++ = entry.path();
    }
}

void zz::dir2zip(const fs::path &path, const options &opts)
{
    const auto dirname = path.filename();

    const auto zip_path = path.parent_path() / (path.filename() + ".zip");
    io::ofstream zip;
    zip.exceptions(ios::failbit | ios::badbit);
    zip.open(zip_path, ios::binary);

    vector<fs::path> files;
    enumerate_files(path, back_inserter(files));
    sort(begin(files), end(files), [](const fs::path &lhs, const fs::path &rhs) {
        return strnatcasecmp(lhs.native(), rhs.native()) < 0;
    });

    vector<pkzip::central_file_header> records;
    for (const auto &file : files) {
        const auto size = fs::file_size(file);
        if (size > std::numeric_limits<decltype(pkzip::local_file_header::compressed_size)>::max())
            throw runtime_error("large file not supported: " + dirname);
        const auto mtime = fs::last_write_time(file);

        pkzip::local_file_header header(opts.charsets.second);
        header.general_purpose_bit_flag = strnatcasecmp(header.charset, "utf8"s) == 0
                                        ? pkzip::general_purpose_bit_flags::use_utf8
                                        : 0;
        header.compressed_size          = static_cast<decltype(header.compressed_size)>(size);
        header.uncompressed_size        = static_cast<decltype(header.uncompressed_size)>(size);
        header.file_name                = fs::relative(file, path);
        replace(begin(header.file_name), end(header.file_name), '\\', '/');
        if (any_of(begin(opts.excludes), end(opts.excludes), [&header](basic_string_view<pkzip::char_type> x)
                   { return x.starts_with('*') ? header.file_name.ends_with(x.substr(1)) : header.file_name == x; }))
            continue;
        tie(header.last_mod_file_date, header.last_mod_file_time) = to_dos_date_time(mtime);

        const streamoff offset = zip.tellp();
        if (offset > numeric_limits<decltype(pkzip::central_file_header::relative_offset_of_local_header)>::max())
            throw runtime_error("large file not supported: " + dirname);
        zip << header;

        crc32_t crc32;
        {
            ifstream is(file, ios::binary);
            for (char buf[8192]; auto n = is.read(buf, sizeof buf).gcount(); ) {
                zip.write(buf, n);
                crc32.process_bytes(buf, static_cast<size_t>(n));
            }
        }
        header.crc32 = crc32();

        const auto next_offset = zip.tellp();
        zip.seekp(offset) << header;
        zip.seekp(next_offset);

        pkzip::central_file_header record(opts.charsets.second);
        record.version_made_by                 = header.version_needed_to_extract | pkzip::version_made_by::msdos;
        record.version_needed_to_extract       = header.version_needed_to_extract;
        record.general_purpose_bit_flag        = header.general_purpose_bit_flag;
        record.last_mod_file_time              = header.last_mod_file_time;
        record.last_mod_file_date              = header.last_mod_file_date;
        record.crc32                           = header.crc32;
        record.compressed_size                 = header.compressed_size;
        record.uncompressed_size               = header.uncompressed_size;
        record.relative_offset_of_local_header = static_cast<decltype(record.relative_offset_of_local_header)>(offset);
        record.file_name                       = header.file_name;
        records.push_back(record);
        if (records.size() > numeric_limits<decltype(pkzip::end_of_central_directory_record::total_number_of_entries_in_the_central_directory)>::max())
            throw runtime_error("too many entries: " + dirname);

        if (!opts.quiet)
            cout << "\r   " << dec << setw(3) << setfill('0') << records.size() << " entries written";
    }
    if (!opts.quiet)
        cout << endl;

    const streamoff directory_offset = zip.tellp();
    using offset_of_directory_type
        = decltype(pkzip::end_of_central_directory_record
            ::offset_of_start_of_central_directory_with_respect_to_the_starting_disk_number);
    if (directory_offset > numeric_limits<offset_of_directory_type>::max())
        throw runtime_error("large file not supported: " + dirname);
    for (const auto &record : records)
        zip << record;
    const streamoff directory_size = zip.tellp() - directory_offset;

    pkzip::end_of_central_directory_record footer;
    footer.total_number_of_entries_in_the_central_directory_on_this_disk
        = static_cast<decltype(footer.total_number_of_entries_in_the_central_directory_on_this_disk)>(records.size());
    footer.total_number_of_entries_in_the_central_directory
        = static_cast<decltype(footer.total_number_of_entries_in_the_central_directory)>(records.size());
    footer.size_of_the_central_directory
        = static_cast<decltype(footer.size_of_the_central_directory)>(directory_size);
    footer.offset_of_start_of_central_directory_with_respect_to_the_starting_disk_number
        = static_cast<decltype(footer.offset_of_start_of_central_directory_with_respect_to_the_starting_disk_number)>(directory_offset);
    zip << footer;

    if (!opts.quiet)
        cout << "   footer written" << endl;

    zip.close();

    fs::last_write_time(zip_path, fs::last_write_time(path));
}
