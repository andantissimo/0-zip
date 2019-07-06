#include <boost/locale/encoding.hpp>
#include <boost/locale/util.hpp>

#include "pkzip_io.h"

using namespace std;

using boost::locale::conv::from_utf;
using boost::locale::conv::to_utf;
using boost::locale::conv::utf_to_utf;
using boost::locale::util::get_system_locale;

static inline auto read(istream &is, void *data, size_t size)
{
    return is.read(static_cast<char *>(data), size)
        && is.gcount() == static_cast<streamsize>(size);
}
template <typename value_type>
static inline auto read(istream &is, value_type &value)
{
    return read(is, &value, sizeof value);
}

static inline auto write(ostream &os, const void *data, size_t size)
{
    os.write(static_cast<const char *>(data), size);
}
template <typename value_type>
static inline auto write(ostream &os, const value_type &value)
{
    write(os, &value, sizeof value);
}

string zz::pkzip::charset = "cp932"; // TODO: resolve from OS environment

istream & zz::pkzip::operator >> (istream &is, local_file_header &header)
{
    if (!read(is, header.signature) || !header ||
        !read(is, header.version_needed_to_extract) ||
        !read(is, header.general_purpose_bit_flag) ||
        !read(is, header.compression_method) ||
        !read(is, header.last_mod_file_time) ||
        !read(is, header.last_mod_file_date) ||
        !read(is, header.crc32) ||
        !read(is, header.compressed_size) ||
        !read(is, header.uncompressed_size) ||
        !read(is, header.file_name_length) || header.file_name_length == 0 ||
        !read(is, header.extra_field_length))
        return is;

    string file_name(header.file_name_length, '\0');
    if (!read(is, &file_name[0], header.file_name_length))
        return is;
    const auto use_utf8 = header.general_purpose_bit_flag & general_purpose_bit_flags::use_utf8;
    header.file_name = use_utf8 ? utf_to_utf<char_type>(file_name) : to_utf<char_type>(file_name, charset);
    if (header.extra_field_length) {
        header.extra_field.resize(header.extra_field_length);
        if (!read(is, &header.extra_field[0], header.extra_field_length))
            return is;
    }
    return is;
}

ostream & zz::pkzip::operator << (ostream &os, const local_file_header &header)
{
    const auto use_utf8 = header.general_purpose_bit_flag & general_purpose_bit_flags::use_utf8;
    const auto file_name = use_utf8 ? utf_to_utf<char>(header.file_name) : from_utf(header.file_name, charset);
    if (file_name.size() > numeric_limits<decltype(header.file_name_length)>::max())
        throw runtime_error("too long file name: " + file_name);
    if (header.extra_field.size() > numeric_limits<decltype(header.extra_field_length)>::max())
        throw runtime_error("too long extra field: " + file_name);

    write(os, header.signature);
    write(os, header.version_needed_to_extract);
    write(os, header.general_purpose_bit_flag);
    write(os, header.compression_method);
    write(os, header.last_mod_file_time);
    write(os, header.last_mod_file_date);
    write(os, header.crc32);
    write(os, header.compressed_size);
    write(os, header.uncompressed_size);
    write(os, static_cast<decltype(header.file_name_length)>(file_name.size()));
    write(os, static_cast<decltype(header.extra_field_length)>(header.extra_field.size()));

    write(os, file_name.data(), file_name.size());
    write(os, header.extra_field.data(), header.extra_field.size());
    return os;
}

istream & zz::pkzip::operator >> (istream &is, central_file_header &header)
{
    if (!read(is, header.signature) || !header ||
        !read(is, header.version_made_by) ||
        !read(is, header.version_needed_to_extract) ||
        !read(is, header.general_purpose_bit_flag) ||
        !read(is, header.compression_method) ||
        !read(is, header.last_mod_file_time) ||
        !read(is, header.last_mod_file_date) ||
        !read(is, header.crc32) ||
        !read(is, header.compressed_size) ||
        !read(is, header.uncompressed_size) ||
        !read(is, header.file_name_length) || header.file_name_length == 0 ||
        !read(is, header.extra_field_length) ||
        !read(is, header.file_comment_length) ||
        !read(is, header.disk_number_start) ||
        !read(is, header.internal_file_attributes) ||
        !read(is, header.external_file_attributes) ||
        !read(is, header.relative_offset_of_local_header))
        return is;

    string file_name(header.file_name_length, '\0');
    if (!is.read(&file_name[0], header.file_name_length))
        return is;
    const auto use_utf8 = header.general_purpose_bit_flag & general_purpose_bit_flags::use_utf8;
    header.file_name = use_utf8 ? utf_to_utf<char_type>(file_name) : to_utf<char_type>(file_name, charset);
    if (header.extra_field_length) {
        header.extra_field.resize(header.extra_field_length);
        if (!read(is, &header.extra_field[0], header.extra_field_length))
            return is;
    }
    if (header.file_comment_length) {
        string file_comment(header.file_comment_length, '\0');
        if (!is.read(&file_comment[0], header.file_comment_length))
            return is;
        header.file_comment = use_utf8 ? utf_to_utf<char_type>(file_comment) : to_utf<char_type>(file_comment, charset);
    }
    return is;
}

ostream & zz::pkzip::operator << (ostream &os, const central_file_header &header)
{
    const auto use_utf8 = header.general_purpose_bit_flag & general_purpose_bit_flags::use_utf8;
    const auto file_name = use_utf8 ? utf_to_utf<char>(header.file_name) : from_utf(header.file_name, charset);
    if (file_name.size() > numeric_limits<decltype(header.file_name_length)>::max())
        throw runtime_error("too long file name: " + file_name);
    if (header.extra_field.size() > numeric_limits<decltype(header.extra_field_length)>::max())
        throw runtime_error("too long extra field: " + file_name);
    const auto file_comment = use_utf8 ? utf_to_utf<char>(header.file_comment) : from_utf(header.file_comment, charset);
    if (file_comment.size() > numeric_limits<decltype(header.file_comment_length)>::max())
        throw runtime_error("too long file comment: " + file_comment);

    write(os, header.signature);
    write(os, header.version_made_by);
    write(os, header.version_needed_to_extract);
    write(os, header.general_purpose_bit_flag);
    write(os, header.compression_method);
    write(os, header.last_mod_file_time);
    write(os, header.last_mod_file_date);
    write(os, header.crc32);
    write(os, header.compressed_size);
    write(os, header.uncompressed_size);
    write(os, static_cast<decltype(header.file_name_length)>(file_name.size()));
    write(os, static_cast<decltype(header.extra_field_length)>(header.extra_field.size()));
    write(os, static_cast<decltype(header.file_comment_length)>(file_comment.size()));
    write(os, header.disk_number_start);
    write(os, header.internal_file_attributes);
    write(os, header.external_file_attributes);
    write(os, header.relative_offset_of_local_header);

    write(os, file_name.data(), file_name.size());
    write(os, header.extra_field.data(), header.extra_field.size());
    write(os, file_comment.data(), file_comment.size());
    return os;
}

istream & zz::pkzip::operator >> (istream &is, end_of_central_directory_record &record)
{
    if (!read(is, record.signature) || !record ||
        !read(is, record.number_of_this_disk) ||
        !read(is, record.number_of_the_disk_with_the_start_of_the_central_directory) ||
        !read(is, record.total_number_of_entries_in_the_central_directory_on_this_disk) ||
        !read(is, record.total_number_of_entries_in_the_central_directory) ||
        !read(is, record.size_of_the_central_directory) ||
        !read(is, record.offset_of_start_of_central_directory_with_respect_to_the_starting_disk_number) ||
        !read(is, record.zip_file_comment_length))
        return is;

    if (record.zip_file_comment_length) {
        record.zip_file_comment.resize(record.zip_file_comment_length);
        if (!is.read(&record.zip_file_comment[0], record.zip_file_comment_length))
            return is;
    }
    return is;
}

ostream & zz::pkzip::operator << (ostream &os, const end_of_central_directory_record &record)
{
    if (record.zip_file_comment.size() > numeric_limits<decltype(record.zip_file_comment_length)>::max())
        throw runtime_error("too long zip file comment: " + record.zip_file_comment);

    write(os, record.signature);
    write(os, record.number_of_this_disk);
    write(os, record.number_of_the_disk_with_the_start_of_the_central_directory);
    write(os, record.total_number_of_entries_in_the_central_directory_on_this_disk);
    write(os, record.total_number_of_entries_in_the_central_directory);
    write(os, record.size_of_the_central_directory);
    write(os, record.offset_of_start_of_central_directory_with_respect_to_the_starting_disk_number);
    write(os, static_cast<decltype(record.zip_file_comment_length)>(record.zip_file_comment.size()));

    write(os, record.zip_file_comment.data(), record.zip_file_comment.size());
    return os;
}
