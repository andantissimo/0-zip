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

string az::pkzip::charset = "cp932"; // TODO: resolve from OS environment

istream & az::pkzip::operator >> (istream &is, local_file_header &header)
{
    if (!read(is, header.signature) || !header)
        return is;
    if (!read(is, header.version_needed_to_extract))
        return is;
    if (!read(is, header.general_purpose_bit_flag))
        return is;
    if (!read(is, header.compression_method))
        return is;
    if (!read(is, header.last_mod_file_time))
        return is;
    if (!read(is, header.last_mod_file_date))
        return is;
    if (!read(is, header.crc32))
        return is;
    if (!read(is, header.compressed_size))
        return is;
    if (!read(is, header.uncompressed_size))
        return is;
    if (!read(is, header.file_name_length) || header.file_name_length == 0)
        return is;
    if (!read(is, header.extra_field_length))
        return is;

    string file_name(header.file_name_length, '\0');
    if (!read(is, &file_name[0], header.file_name_length))
        return is;
    header.file_name = (header.general_purpose_bit_flag & general_purpose_bit_flags::use_utf8)
        ? utf_to_utf<char_type>(file_name)
        : to_utf<char_type>(file_name, charset);
    if (header.extra_field_length) {
        header.extra_field.resize(header.extra_field_length);
        if (!read(is, &header.extra_field[0], header.extra_field_length))
            return is;
    }
    return is;
}

ostream & az::pkzip::operator << (ostream &os, const local_file_header &header)
{
    const auto file_name = (header.general_purpose_bit_flag & general_purpose_bit_flags::use_utf8)
        ? utf_to_utf<char>(header.file_name)
        : from_utf(header.file_name, charset);
    if (file_name.size() > numeric_limits<decltype(header.file_name_length)>::max())
        throw runtime_error("too long file name: " + file_name);
    if (header.extra_field.size() > numeric_limits<decltype(header.extra_field_length)>::max())
        throw runtime_error("invalid extra field: " + file_name);

    const auto file_name_length = static_cast<decltype(header.file_name_length)>(file_name.size());
    const auto extra_field_length = static_cast<decltype(header.extra_field_length)>(header.extra_field.size());

    write(os, header.signature);
    write(os, header.version_needed_to_extract);
    write(os, header.general_purpose_bit_flag);
    write(os, header.compression_method);
    write(os, header.last_mod_file_time);
    write(os, header.last_mod_file_date);
    write(os, header.crc32);
    write(os, header.compressed_size);
    write(os, header.uncompressed_size);
    write(os, file_name_length);
    write(os, extra_field_length);

    write(os, file_name.data(), file_name_length);
    if (extra_field_length)
        write(os, header.extra_field.data(), extra_field_length);
    return os;
}

istream & az::pkzip::operator >> (istream &is, central_file_header &header)
{
    if (!read(is, header.signature) || !header)
        return is;
    if (!read(is, header.version_made_by))
        return is;
    if (!read(is, header.version_needed_to_extract))
        return is;
    if (!read(is, header.general_purpose_bit_flag))
        return is;
    if (!read(is, header.compression_method))
        return is;
    if (!read(is, header.last_mod_file_time))
        return is;
    if (!read(is, header.last_mod_file_date))
        return is;
    if (!read(is, header.crc32))
        return is;
    if (!read(is, header.compressed_size))
        return is;
    if (!read(is, header.uncompressed_size))
        return is;
    if (!read(is, header.file_name_length) || header.file_name_length == 0)
        return is;
    if (!read(is, header.extra_field_length))
        return is;
    if (!read(is, header.file_comment_length))
        return is;
    if (!read(is, header.disk_number_start))
        return is;
    if (!read(is, header.internal_file_attributes))
        return is;
    if (!read(is, header.external_file_attributes))
        return is;
    if (!read(is, header.relative_offset_of_local_header))
        return is;

    string file_name(header.file_name_length, '\0');
    if (!is.read(&file_name[0], header.file_name_length))
        return is;
    header.file_name = (header.general_purpose_bit_flag & general_purpose_bit_flags::use_utf8)
        ? utf_to_utf<char_type>(file_name)
        : to_utf<char_type>(file_name, charset);
    if (header.extra_field_length) {
        header.extra_field.resize(header.extra_field_length);
        if (!read(is, &header.extra_field[0], header.extra_field_length))
            return is;
    }
    if (header.file_comment_length) {
        string file_comment(header.file_comment_length, '\0');
        if (!is.read(&file_comment[0], header.file_comment_length))
            return is;
        header.file_comment = (header.general_purpose_bit_flag & general_purpose_bit_flags::use_utf8)
            ? utf_to_utf<char_type>(file_comment)
            : to_utf<char_type>(file_comment, charset);
    }
    return is;
}

ostream & az::pkzip::operator << (ostream &os, const central_file_header &header)
{
    const auto file_name = (header.general_purpose_bit_flag & general_purpose_bit_flags::use_utf8)
        ? utf_to_utf<char>(header.file_name)
        : from_utf(header.file_name, charset);
    if (file_name.size() > numeric_limits<decltype(header.file_name_length)>::max())
        throw runtime_error("too long file name: " + file_name);
    if (header.extra_field.size() > numeric_limits<decltype(header.extra_field_length)>::max())
        throw runtime_error("invalid extra field: " + file_name);
    const auto file_comment = (header.general_purpose_bit_flag & general_purpose_bit_flags::use_utf8)
        ? utf_to_utf<char>(header.file_comment)
        : from_utf(header.file_comment, charset);
    if (file_comment.size() > numeric_limits<decltype(header.file_comment_length)>::max())
        throw runtime_error("too long file comment: " + file_comment);

    const auto file_name_length = static_cast<decltype(header.file_name_length)>(file_name.length());
    const auto extra_field_length = static_cast<decltype(header.extra_field_length)>(header.extra_field.size());
    const auto file_comment_length = static_cast<decltype(header.file_comment_length)>(file_comment.length());

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
    write(os, file_name_length);
    write(os, extra_field_length);
    write(os, file_comment_length);
    write(os, header.disk_number_start);
    write(os, header.internal_file_attributes);
    write(os, header.external_file_attributes);
    write(os, header.relative_offset_of_local_header);

    write(os, file_name.data(), file_name_length);
    if (extra_field_length)
        write(os, header.extra_field.data(), extra_field_length);
    if (file_comment_length)
        write(os, file_comment.data(), file_comment_length);
    return os;
}

istream & az::pkzip::operator >> (istream &is, end_of_central_directory_record &record)
{
    if (!read(is, record.signature) || !record)
        return is;
    if (!read(is, record.number_of_this_disk))
        return is;
    if (!read(is, record.number_of_the_disk_with_the_start_of_the_central_directory))
        return is;
    if (!read(is, record.total_number_of_entries_in_the_central_directory_on_this_disk))
        return is;
    if (!read(is, record.total_number_of_entries_in_the_central_directory))
        return is;
    if (!read(is, record.size_of_the_central_directory))
        return is;
    if (!read(is, record.offset_of_start_of_central_directory_with_respect_to_the_starting_disk_number))
        return is;
    if (!read(is, record.zip_file_comment_length))
        return is;

    if (record.zip_file_comment_length) {
        record.zip_file_comment.resize(record.zip_file_comment_length);
        if (!is.read(&record.zip_file_comment[0], record.zip_file_comment_length))
            return is;
    }
    return is;
}

ostream & az::pkzip::operator << (ostream &os, const end_of_central_directory_record &record)
{
    if (record.zip_file_comment.size() > numeric_limits<decltype(record.zip_file_comment_length)>::max())
        throw runtime_error("too long zip file comment: " + record.zip_file_comment);
    const auto zip_file_comment_length
        = static_cast<decltype(record.zip_file_comment_length)>(record.zip_file_comment.length());

    write(os, record.signature);
    write(os, record.number_of_this_disk);
    write(os, record.number_of_the_disk_with_the_start_of_the_central_directory);
    write(os, record.total_number_of_entries_in_the_central_directory_on_this_disk);
    write(os, record.total_number_of_entries_in_the_central_directory);
    write(os, record.size_of_the_central_directory);
    write(os, record.offset_of_start_of_central_directory_with_respect_to_the_starting_disk_number);
    write(os, zip_file_comment_length);

    if (zip_file_comment_length)
        write(os, record.zip_file_comment.data(), zip_file_comment_length);
    return os;
}
