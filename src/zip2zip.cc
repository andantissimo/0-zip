#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4706)
#endif
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "path_ops.h"
#include "pkzip_io.h"
#include "strnatcmp.h"
#include "trash.h"

#include "zip2zip.h"

using namespace zz;
using namespace std;

using boost::iostreams::filtering_istream;
using boost::iostreams::zlib_decompressor;
using boost::iostreams::zlib_params;

template <typename size_type>
static inline auto & copy_n(io::ifstream &source, size_type count, io::ofstream &dest)
{
    constexpr size_type buffer_size = 8192;
    char buffer[buffer_size];
    for (streamsize read; (read = source.read(buffer, min(buffer_size, count)).gcount()) > 0; count -= static_cast<size_type>(read))
        dest.write(buffer, read);
    return dest;
}

void zz::zip2zip(const fs::path &path, const options &opts)
{
    const auto filename = path.filename();
    const auto filesize = fs::file_size(path);

    io::ifstream zip;
    zip.exceptions(ios::failbit | ios::badbit);
    zip.open(path, ios::binary);

    struct entry_t
    {
        pkzip::local_file_header header;
        streamoff                offset;
    };
    vector<entry_t> entries;
    for (pkzip::local_file_header header(opts.charset.first); zip >> header && header; zip.seekg(entries.back().offset + header.compressed_size)) {
        if (header.general_purpose_bit_flag & pkzip::general_purpose_bit_flags::file_is_encrypted)
            throw runtime_error("encryption not supported: " + filename);
        if (header.general_purpose_bit_flag & pkzip::general_purpose_bit_flags::has_data_descriptor)
            throw runtime_error("data descriptor not supported: " + filename);
        const streamoff offset = zip.tellg();
        if (static_cast<decltype(filesize)>(offset + header.compressed_size) > filesize)
            break;
        header.charset = opts.charset.second;
        entries.push_back(entry_t{ header, offset });
        using total_number_of_entries_type
            = decltype(pkzip::end_of_central_directory_record::total_number_of_entries_in_the_central_directory);
        if (entries.size() > numeric_limits<total_number_of_entries_type>::max())
            throw runtime_error("too many entries: " + filename);

        if (!opts.quiet)
            cout << "\r   " << dec << setw(3) << setfill('0') << entries.size() << " entries read";
    }
    if (!opts.quiet)
        cout << endl;

    zip.clear();

    sort(begin(entries), end(entries), [](const auto &lhs, const auto &rhs) {
        return strnatcasecmp(lhs.header.file_name, rhs.header.file_name) < 0;
    });

    const auto tmp_path = path.parent_path() / path.filename().replace_extension(".tmp");
    io::ofstream tmp;
    tmp.exceptions(ios::failbit | ios::badbit);
    tmp.open(tmp_path, ios::binary);

    const auto tmp_name = tmp_path.filename();

    vector<pkzip::central_file_header> records;
    for (auto &entry : entries) {
        const streamoff offset = tmp.tellp();
        if (offset > numeric_limits<decltype(pkzip::central_file_header::relative_offset_of_local_header)>::max())
            throw runtime_error("large file not supported: " + filename);

        pkzip::central_file_header record(opts.charset.second);
        record.version_made_by                 = pkzip::version_made_by::msdos | entry.header.version_needed_to_extract;
        record.version_needed_to_extract       = entry.header.version_needed_to_extract;
        record.general_purpose_bit_flag        = entry.header.general_purpose_bit_flag;
        record.compression_method              = entry.header.compression_method;
        record.last_mod_file_time              = entry.header.last_mod_file_time;
        record.last_mod_file_date              = entry.header.last_mod_file_date;
        record.crc32                           = entry.header.crc32;
        record.compressed_size                 = entry.header.compressed_size;
        record.uncompressed_size               = entry.header.uncompressed_size;
        record.relative_offset_of_local_header = static_cast<decltype(record.relative_offset_of_local_header)>(offset);
        record.file_name                       = entry.header.file_name;
        record.extra_field                     = entry.header.extra_field;

        zip.seekg(entry.offset);
        if (entry.header.compression_method == pkzip::compression_method::deflated) {
            vector<char> buf(entry.header.uncompressed_size);
            zlib_params z;
            z.noheader = true;
            filtering_istream dec;
            dec.push(zlib_decompressor(z));
            dec.push(zip);
            dec.read(data(buf), size(buf));
            record.compression_method = entry.header.compression_method = pkzip::compression_method::stored;
            record.compressed_size    = entry.header.compressed_size    = entry.header.uncompressed_size;
            tmp << entry.header;
            tmp.write(data(buf), size(buf));
        } else {
            tmp << entry.header;
            copy_n(zip, entry.header.compressed_size, tmp);
        }

        records.push_back(record);

        if (!opts.quiet)
            cout << "\r   " << dec << setw(3) << setfill('0') << records.size() << " entries written";
    }
    if (!opts.quiet)
        cout << endl;

    zip.close();

    const streamoff directory_offset = tmp.tellp();
    using offset_of_directory_type
        = decltype(pkzip::end_of_central_directory_record
            ::offset_of_start_of_central_directory_with_respect_to_the_starting_disk_number);
    if (directory_offset > numeric_limits<offset_of_directory_type>::max())
        throw runtime_error("large file not supported: " + filename);
    for (const auto &record : records)
        tmp << record;
    const streamoff directory_size = tmp.tellp() - directory_offset;

    pkzip::end_of_central_directory_record footer;
    footer.total_number_of_entries_in_the_central_directory_on_this_disk
        = static_cast<decltype(footer.total_number_of_entries_in_the_central_directory_on_this_disk)>(records.size());
    footer.total_number_of_entries_in_the_central_directory
        = static_cast<decltype(footer.total_number_of_entries_in_the_central_directory)>(records.size());
    footer.size_of_the_central_directory
        = static_cast<decltype(footer.size_of_the_central_directory)>(directory_size);
    footer.offset_of_start_of_central_directory_with_respect_to_the_starting_disk_number
        = static_cast<decltype(footer.offset_of_start_of_central_directory_with_respect_to_the_starting_disk_number)>(directory_offset);
    tmp << footer;

    if (!opts.quiet)
        cout << "   footer written" << endl;

    tmp.close();

    const auto mtime = fs::last_write_time(path);

    fs::trash(path);
    if (!opts.quiet)
        cout << "   trashed" << endl;

    fs::rename(tmp_path, path);
    if (!opts.quiet)
        cout << "   renamed" << endl;

    fs::last_write_time(path, mtime);
}
