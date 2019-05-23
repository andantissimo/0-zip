#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "config.h"

namespace az::pkzip
{
    using char_type   = fs::path::value_type;
    using string_type = fs::path::string_type;
    using binary_type = std::vector<uint8_t>;

    constexpr uint32_t local_file_header_signature = 'P' | 'K' << 8 | 3 << 16 | 4 << 24;

    struct local_file_header
    {
        uint32_t signature;
        uint16_t version_needed_to_extract;
        uint16_t general_purpose_bit_flag;
        uint16_t compression_method;
        uint16_t last_mod_file_time;
        uint16_t last_mod_file_date;
        uint32_t crc32;
        uint32_t compressed_size;
        uint32_t uncompressed_size;
        uint16_t file_name_length;
        uint16_t extra_field_length;

        string_type file_name;
        binary_type extra_field;

        explicit operator bool () const noexcept
        {
            return signature == local_file_header_signature;
        }
        bool operator ! () const noexcept
        {
            return signature != local_file_header_signature;
        }
    };

    constexpr uint32_t central_file_header_signature = 'P' | 'K' << 8 | 1 << 16 | 2 << 24;

    struct central_file_header
    {
        uint32_t signature;
        uint16_t version_made_by;
        uint16_t version_needed_to_extract;
        uint16_t general_purpose_bit_flag;
        uint16_t compression_method;
        uint16_t last_mod_file_time;
        uint16_t last_mod_file_date;
        uint32_t crc32;
        uint32_t compressed_size;
        uint32_t uncompressed_size;
        uint16_t file_name_length;
        uint16_t extra_field_length;
        uint16_t file_comment_length;
        uint16_t disk_number_start;
        uint16_t internal_file_attributes;
        uint32_t external_file_attributes;
        uint32_t relative_offset_of_local_header;

        string_type file_name;
        binary_type extra_field;
        string_type file_comment;

        explicit operator bool() const noexcept
        {
            return signature == central_file_header_signature;
        }
        bool operator ! () const noexcept
        {
            return signature != central_file_header_signature;
        }
    };

    constexpr uint32_t end_of_central_directory_record_signature = 'P' | 'K' << 8 | 5 << 16 | 6 << 24;

    struct end_of_central_directory_record
    {
        uint32_t signature;
        uint16_t number_of_this_disk;
        uint16_t number_of_the_disk_with_the_start_of_the_central_directory;
        uint16_t total_number_of_entries_in_the_central_directory_on_this_disk;
        uint16_t total_number_of_entries_in_the_central_directory;
        uint32_t size_of_the_central_directory;
        uint32_t offset_of_start_of_central_directory_with_respect_to_the_starting_disk_number;
        uint16_t zip_file_comment_length;

        std::string zip_file_comment;

        explicit operator bool() const noexcept
        {
            return signature == end_of_central_directory_record_signature;
        }
        bool operator ! () const noexcept
        {
            return signature != end_of_central_directory_record_signature;
        }
    };

    namespace version_made_by
    {
        constexpr uint16_t msdos   =  0 << 8; // FAT, FAT32
        constexpr uint16_t unix    =  3 << 8;
        constexpr uint16_t windows = 10 << 8; // NTFS
        constexpr uint16_t osx     = 19 << 8;
    }

    namespace version_needed_to_extract
    {
        constexpr uint16_t default_value           = 10;
        constexpr uint16_t use_deflate_compression = 20;
    }

    namespace general_purpose_bit_flags
    {
        constexpr uint16_t file_is_encrypted   = 1 <<  0;
        constexpr uint16_t has_data_descriptor = 1 <<  3;
        constexpr uint16_t use_utf8            = 1 << 11;
    }

    namespace compression_method
    {
        constexpr uint16_t stored   = 0;
        constexpr uint16_t deflated = 8;
    }
}