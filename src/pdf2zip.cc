#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string_view>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244 4245)
#endif
#include <boost/crc.hpp>
#include <boost/interprocess/streams/bufferstream.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "dostime.h"
#include "path_ops.h"
#include "pkzip_io.h"
#include "strnatcmp.h"

#include "pdf2zip.h"

using namespace zz;
using namespace std;

using boost::interprocess::ibufferstream;

static inline auto is_crlf(char ch)
{
    return ch == '\r' || ch == '\n';
}

static inline auto compute_crc32(const void *data, size_t size)
{
    boost::crc_32_type crc;
    crc.process_bytes(data, size);
    return crc.checksum();
}

static inline auto make_file_name(size_t object_num, const char *extension)
{
    basic_ostringstream<pkzip::char_type> ss;
    ss << setw(3) << setfill<pkzip::char_type>('0') << object_num << extension;
    return ss.str();
}

void zz::pdf2zip(const fs::path &path, const options &opts)
{
    const auto filename = path.filename();
    const auto mtime = fs::last_write_time(path);

    string pdf(static_cast<string::size_type>(fs::file_size(path)), '\0');
    {
        io::ifstream file;
        file.open(path, ios::binary);
        file.read(&pdf[0], pdf.size());
    }

    auto eof = pdf.rfind("%%EOF");
    if (eof == string::npos)
        throw runtime_error("%%EOF not found: " + filename);

    struct object_t
    {
        size_t number, begin, end;
    };
    vector<object_t> objects;
    {
        auto startxref = pdf.rfind("startxref", eof - 1);
        if (startxref == string::npos)
            throw runtime_error("startxref not found: " + filename);
        size_t xref = 0;
        ibufferstream(pdf.data() + startxref + 9, eof - startxref - 9) >> xref;
        if (xref == 0 || pdf.compare(xref, 4, "xref") != 0)
            throw runtime_error("invalid startxref: " + filename);
        auto trailer = pdf.find("trailer", xref + 4);
        if (trailer == string::npos)
            throw runtime_error("trailer not found: " + filename);
        ibufferstream xrefs(pdf.data() + xref + 4, trailer - xref - 4);
        size_t number = 0, count = 0;
        xrefs >> number >> count;
        for (size_t i = 0; i < count; i++) {
            size_t begin, revision;
            char flag;
            xrefs >> begin >> revision >> flag;
            if (flag == 'f')
                continue;
            objects.push_back(object_t{ number + i, begin });
        }
        if (objects.empty())
            throw runtime_error("no object found: " + filename);

        sort(begin(objects), end(objects), [](const auto &lhs, const auto &rhs) { return lhs.begin < rhs.begin; });
        for (size_t i = 1; i < objects.size(); i++)
            objects[i - 1].end = objects[i].begin;
        objects[objects.size() - 1].end = startxref;
    }

    sort(begin(objects), end(objects), [](const auto &lhs, const auto &rhs) { return lhs.number < rhs.number; });

    struct entry_t
    {
        pkzip::local_file_header header;
        string_view              stream;
    };
    vector<entry_t> entries;
    for (const auto &object : objects) {
        const auto object_view = string_view(pdf.data() + object.begin, object.end - object.begin);
        auto stream = object_view.find("stream");
        if (stream == string_view::npos)
            continue;
        for (stream += 6; stream < object_view.size() && is_crlf(object_view[stream]); stream++)
            continue;
        auto endstream = object_view.rfind("endstream");
        if (endstream == string_view::npos || endstream <= stream)
            continue;
        for (; stream < endstream && is_crlf(object_view[endstream]); endstream--)
            continue;
        const auto meta = object_view.substr(0, stream);

        // JPEG
        if (meta.find("/DCTDecode") != string_view::npos) {
            const auto stream_view = object_view.substr(stream, endstream - stream);
            if (stream_view.size() > numeric_limits<decltype(pkzip::local_file_header::compressed_size)>::max())
                throw runtime_error("large file not supported: " + filename);

            pkzip::local_file_header header(opts.charsets.second);
            header.general_purpose_bit_flag = strnatcasecmp(header.charset, "utf8"s) == 0
                                            ? pkzip::general_purpose_bit_flags::use_utf8
                                            : 0;
            header.crc32                    = compute_crc32(stream_view.data(), stream_view.size());
            header.compressed_size          = static_cast<decltype(header.compressed_size)>(stream_view.size());
            header.uncompressed_size        = static_cast<decltype(header.uncompressed_size)>(stream_view.size());
            header.file_name                = make_file_name(1 + entries.size(), ".jpg");
            tie(header.last_mod_file_date, header.last_mod_file_time) = to_dos_date_time(mtime);
            entries.push_back({ header, stream_view });

            if (!opts.quiet)
                cout << "\r   " << dec << setw(3) << setfill('0') << entries.size() << " entries found";
            continue;
        }

        // TIFF (Monochrome)
        //if (meta.find("/CCITTFaxDecode") != string_view::npos) {
        //    continue;
        //}
    }
    if (!opts.quiet)
        cout << endl;

    const auto zip_path = path.parent_path() / path.filename().replace_extension(".zip");
    io::ofstream zip;
    zip.exceptions(ios::failbit | ios::badbit);
    zip.open(zip_path, ios::binary);

    const auto zip_name = zip_path.filename();

    vector<pkzip::central_file_header> records;
    for (const auto &entry : entries) {
        const streamoff offset = zip.tellp();
        if (offset > numeric_limits<decltype(pkzip::central_file_header::relative_offset_of_local_header)>::max())
            throw runtime_error("large file not supported: " + filename);

        pkzip::central_file_header record(opts.charsets.second);
        record.version_made_by                 = entry.header.version_needed_to_extract | pkzip::version_made_by::msdos;
        record.version_needed_to_extract       = entry.header.version_needed_to_extract;
        record.general_purpose_bit_flag        = entry.header.general_purpose_bit_flag;
        record.last_mod_file_time              = entry.header.last_mod_file_time;
        record.last_mod_file_date              = entry.header.last_mod_file_date;
        record.crc32                           = entry.header.crc32;
        record.compressed_size                 = entry.header.compressed_size;
        record.uncompressed_size               = entry.header.uncompressed_size;
        record.relative_offset_of_local_header = static_cast<decltype(record.relative_offset_of_local_header)>(offset);
        record.file_name                       = entry.header.file_name;
        records.push_back(record);

        zip << entry.header;
        zip.write(entry.stream.data(), entry.stream.size());

        if (!opts.quiet)
            cout << "\r   " << dec << setw(3) << setfill('0') << records.size() << " entries written";
    }
    if (!opts.quiet)
        cout << endl;

    pdf.clear();

    const streamoff directory_offset = zip.tellp();
    using offset_of_directory_type
        = decltype(pkzip::end_of_central_directory_record
            ::offset_of_start_of_central_directory_with_respect_to_the_starting_disk_number);
    if (directory_offset > numeric_limits<offset_of_directory_type>::max())
        throw runtime_error("large file not supported: " + filename);
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

    fs::last_write_time(zip_path, mtime);
}
