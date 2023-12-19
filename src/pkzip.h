#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "config.h"

namespace zz::pkzip
{
    using char_type   = fs::path::value_type;
    using string_type = fs::path::string_type;
    using binary_type = std::vector<uint8_t>;

    namespace version_made_by
    {
        constexpr uint16_t msdos =  0 << 8; // FAT, FAT32
        constexpr uint16_t unix  =  3 << 8;
        constexpr uint16_t ntfs  = 10 << 8;
    }

    namespace version_needed_to_extract
    {
        constexpr uint16_t default_value = 10;
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

    namespace msdos
    {
        constexpr uint32_t file_attribute_directory = 0x00000010;
    }

    constexpr uint32_t local_file_header_signature = 'P' | 'K' << 8 | 3 << 16 | 4 << 24;

    struct local_file_header
    {
        uint32_t signature                 = local_file_header_signature;
        uint16_t version_needed_to_extract = version_needed_to_extract::default_value;
        uint16_t general_purpose_bit_flag  = 0;
        uint16_t compression_method        = compression_method::stored;
        uint16_t last_mod_file_time        = 0;
        uint16_t last_mod_file_date        = 0;
        uint32_t crc32                     = 0;
        uint32_t compressed_size           = 0;
        uint32_t uncompressed_size         = 0;
        uint16_t file_name_length          = 0;
        uint16_t extra_field_length        = 0;

        std::string charset;

        string_type file_name   = {};
        binary_type extra_field = {};

        explicit local_file_header(const std::string &charset)
            : charset(charset)
        {
        }
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
        uint32_t signature                       = central_file_header_signature;
        uint16_t version_made_by                 = version_needed_to_extract::default_value | version_made_by::msdos;
        uint16_t version_needed_to_extract       = version_needed_to_extract::default_value;
        uint16_t general_purpose_bit_flag        = 0;
        uint16_t compression_method              = compression_method::stored;
        uint16_t last_mod_file_time              = 0;
        uint16_t last_mod_file_date              = 0;
        uint32_t crc32                           = 0;
        uint32_t compressed_size                 = 0;
        uint32_t uncompressed_size               = 0;
        uint16_t file_name_length                = 0;
        uint16_t extra_field_length              = 0;
        uint16_t file_comment_length             = 0;
        uint16_t disk_number_start               = 0;
        uint16_t internal_file_attributes        = 0;
        uint32_t external_file_attributes        = 0;
        uint32_t relative_offset_of_local_header = 0;

        std::string charset;

        string_type file_name    = {};
        binary_type extra_field  = {};
        string_type file_comment = {};

        explicit central_file_header(const std::string &charset)
            : charset(charset)
        {
        }
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
        uint32_t signature                                                                     = end_of_central_directory_record_signature;
        uint16_t number_of_this_disk                                                           = 0;
        uint16_t number_of_the_disk_with_the_start_of_the_central_directory                    = 0;
        uint16_t total_number_of_entries_in_the_central_directory_on_this_disk                 = 0;
        uint16_t total_number_of_entries_in_the_central_directory                              = 0;
        uint32_t size_of_the_central_directory                                                 = 0;
        uint32_t offset_of_start_of_central_directory_with_respect_to_the_starting_disk_number = 0;
        uint16_t zip_file_comment_length                                                       = 0;

        std::string zip_file_comment = {};

        explicit operator bool() const noexcept
        {
            return signature == end_of_central_directory_record_signature;
        }
        bool operator ! () const noexcept
        {
            return signature != end_of_central_directory_record_signature;
        }
    };
}
