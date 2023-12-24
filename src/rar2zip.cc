#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244 4245)
#endif
#include <boost/crc.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "dll.h"
#include "path_ops.h"
#include "pkzip_io.h"
#include "strnatcmp.h"

#include "rar2zip.h"

using namespace zz;
using namespace std;

using crc32_t = boost::crc_32_type;

#define ERAR_SUCCESS             0
#define ERAR_END_ARCHIVE        10
#define ERAR_NO_MEMORY          11
#define ERAR_BAD_DATA           12
#define ERAR_BAD_ARCHIVE        13
#define ERAR_UNKNOWN_FORMAT     14
#define ERAR_EOPEN              15
#define ERAR_ECREATE            16
#define ERAR_ECLOSE             17
#define ERAR_EREAD              18
#define ERAR_EWRITE             19
#define ERAR_SMALL_BUF          20
#define ERAR_UNKNOWN            21
#define ERAR_MISSING_PASSWORD   22
#define ERAR_EREFERENCE         23
#define ERAR_BAD_PASSWORD       24

#define RAR_OM_LIST              0
#define RAR_OM_EXTRACT           1
#define RAR_OM_LIST_INCSPLIT     2

#define RAR_SKIP              0
#define RAR_TEST              1
#define RAR_EXTRACT           2

#define RAR_VOL_ASK           0
#define RAR_VOL_NOTIFY        1

#define RAR_DLL_VERSION       8

#define RAR_HASH_NONE         0
#define RAR_HASH_CRC32        1
#define RAR_HASH_BLAKE2       2

#define RHDF_SPLITBEFORE 0x01
#define RHDF_SPLITAFTER  0x02
#define RHDF_ENCRYPTED   0x04
#define RHDF_SOLID       0x10
#define RHDF_DIRECTORY   0x20

#ifndef _WIN32
#define CALLBACK
#define PASCAL
#define HANDLE void *
#endif

typedef int (CALLBACK *UNRARCALLBACK)(uint32_t /*msg*/, intptr_t /*UserData*/, intptr_t, intptr_t);

#pragma pack(push, 1)
struct RAROpenArchiveDataEx
{
    char         *ArcName;
    wchar_t      *ArcNameW;
    uint32_t      OpenMode;
    uint32_t      OpenResult;
    char         *CmtBuf;
    uint32_t      CmtBufSize;
    uint32_t      CmtSize;
    uint32_t      CmtState;
    uint32_t      Flags;
    UNRARCALLBACK Callback;
    intptr_t      UserData;
    uint32_t      Reserved[28];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct RARHeaderDataEx
{
    char     ArcName[1024];
    wchar_t  ArcNameW[1024];
    char     FileName[1024];
    wchar_t  FileNameW[1024];
    uint32_t Flags;
    uint32_t PackSize;
    uint32_t PackSizeHigh;
    uint32_t UnpSize;
    uint32_t UnpSizeHigh;
    uint32_t HostOS;
    uint32_t FileCRC;
    uint32_t FileTime;
    uint32_t UnpVer;
    uint32_t Method;
    uint32_t FileAttr;
    char    *CmtBuf;
    uint32_t CmtBufSize;
    uint32_t CmtSize;
    uint32_t CmtState;
    uint32_t DictSize;
    uint32_t HashType;
    char     Hash[32];
    uint32_t RedirType;
    wchar_t *RedirName;
    uint32_t RedirNameSize;
    uint32_t DirTarget;
    uint32_t MtimeLow;
    uint32_t MtimeHigh;
    uint32_t CtimeLow;
    uint32_t CtimeHigh;
    uint32_t AtimeLow;
    uint32_t AtimeHigh;
    uint32_t Reserved[988];
};
#pragma pack(pop)

enum UNRARCALLBACK_MESSAGES
{
    UCM_CHANGEVOLUME,
    UCM_PROCESSDATA,
    UCM_NEEDPASSWORD,
    UCM_CHANGEVOLUMEW,
    UCM_NEEDPASSWORDW,
};

class libunrar : public dll
{
public:
    libunrar() noexcept
#if defined _WIN64
        : dll("unrar64.dll")
#elif defined _WIN32
        : dll("unrar.dll")
#elif defined __APPLE__
        : dll("libunrar.dylib")
#else
        : dll("libunrar.so")
#endif
    {
        RAROpenArchiveEx = symbol<decltype(RAROpenArchiveEx)>("RAROpenArchiveEx");
        RARCloseArchive  = symbol<decltype(RARCloseArchive)>("RARCloseArchive");
        RARReadHeaderEx  = symbol<decltype(RARReadHeaderEx)>("RARReadHeaderEx");
        RARProcessFileW  = symbol<decltype(RARProcessFileW)>("RARProcessFileW");
        RARSetCallback   = symbol<decltype(RARSetCallback)>("RARSetCallback");
    }
    explicit operator bool () const noexcept
    {
        return RAROpenArchiveEx
            && RARCloseArchive
            && RARReadHeaderEx
            && RARProcessFileW
            && RARSetCallback;
    }
    HANDLE (PASCAL *RAROpenArchiveEx)(RAROpenArchiveDataEx *) = nullptr;
    int    (PASCAL *RARCloseArchive)(HANDLE) = nullptr;
    int    (PASCAL *RARReadHeaderEx)(HANDLE, RARHeaderDataEx *) = nullptr;
    int    (PASCAL *RARProcessFileW)(HANDLE, int, wchar_t *, wchar_t *) = nullptr;
    void   (PASCAL *RARSetCallback)(HANDLE, UNRARCALLBACK, intptr_t) = nullptr;
};

bool zz::rar_exists()
{
    libunrar unrar;
    return !!unrar;
}

void zz::rar2zip(const fs::path &path, const options &opts)
{
    libunrar unrar;
    if (!unrar)
        throw runtime_error("libunrar not found");

    const auto filename = path.filename();

    unique_handle<HANDLE, decltype(unrar.RARCloseArchive)> hArchive(unrar.RARCloseArchive);
    {
        RAROpenArchiveDataEx rarOpenData = {};
#ifdef _UNICODE
        rarOpenData.ArcNameW = const_cast<wchar_t *>(path.c_str());
#else
        rarOpenData.ArcName  = const_cast<char *>(path.c_str());
#endif
        rarOpenData.OpenMode = RAR_OM_EXTRACT;
        hArchive = unrar.RAROpenArchiveEx(&rarOpenData);
        if (rarOpenData.OpenResult != ERAR_SUCCESS)
            throw runtime_error("failed to open: " + filename);
    }

    const auto zip_path = path.parent_path() / path.filename().replace_extension(".zip");
    io::ofstream zip;
    zip.exceptions(ios::failbit | ios::badbit);
    zip.open(zip_path, ios::binary);

    vector<pkzip::central_file_header> records;
    for (;;) {
        RARHeaderDataEx rarHeaderData = {};
        if (unrar.RARReadHeaderEx(hArchive, &rarHeaderData) != ERAR_SUCCESS)
            break;
        if (rarHeaderData.Flags & RHDF_ENCRYPTED)
            throw runtime_error("encryption not supported: " + filename);
        if (rarHeaderData.Flags & RHDF_DIRECTORY)
            continue;
        if (rarHeaderData.UnpSizeHigh != 0)
            throw runtime_error("large file not supported: " + filename);

        pkzip::local_file_header header(opts.charsets.second);
        header.general_purpose_bit_flag = strnatcasecmp(header.charset, "utf8"s) == 0
                                        ? pkzip::general_purpose_bit_flags::use_utf8
                                        : 0;
        header.last_mod_file_time       = static_cast<uint16_t>(rarHeaderData.FileTime >>  0 & 0xFFFF);
        header.last_mod_file_date       = static_cast<uint16_t>(rarHeaderData.FileTime >> 16 & 0xFFFF);
        header.compressed_size          = rarHeaderData.UnpSize;
        header.uncompressed_size        = rarHeaderData.UnpSize;
#ifdef _UNICODE
        header.file_name                = rarHeaderData.FileNameW;
#else
        header.file_name                = rarHeaderData.FileName;
#endif
        replace(begin(header.file_name), end(header.file_name), '\\', '/');
        if (any_of(begin(opts.excludes), end(opts.excludes), [&header](basic_string_view<pkzip::char_type> x)
                   { return x.starts_with('*') ? header.file_name.ends_with(x.substr(1)) : header.file_name == x; }))
            continue;

        const streamoff offset = zip.tellp();
        if (offset > numeric_limits<decltype(pkzip::central_file_header::relative_offset_of_local_header)>::max())
            throw runtime_error("large file not supported: " + filename);
        zip << header;

        struct context_t
        {
            context_t(ostream &s)
                : stream(s)
            {
            }
            void write(const void *buffer, size_t count)
            {
                stream.write(static_cast<const char *>(buffer), count);
                crc32.process_bytes(buffer, count);
            }
            ostream &stream;
            crc32_t  crc32;
        } context(zip);
        unrar.RARSetCallback(hArchive, [](const uint32_t msg, intptr_t user_data, intptr_t p1, intptr_t p2) {
            switch (msg) {
            case UCM_PROCESSDATA:
                reinterpret_cast<context_t *>(user_data)
                    ->write(reinterpret_cast<const void *>(p1), static_cast<size_t>(p2));
                return 1;
            }
            return -1;
        }, reinterpret_cast<intptr_t>(&context));
        if (unrar.RARProcessFileW(hArchive, RAR_TEST, nullptr, nullptr) != ERAR_SUCCESS)
            throw runtime_error("failed to read: " + filename);

        header.crc32 = context.crc32();
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
            throw runtime_error("too many entries: " + filename);

        if (!opts.quiet)
            cout << "\r   " << dec << setw(3) << setfill('0') << records.size() << " entries written";
    }
    if (!opts.quiet)
        cout << endl;

    hArchive = nullptr;

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

    fs::last_write_time(zip_path, fs::last_write_time(path));
}
