#pragma once

#include "config.h"

namespace zz::fs
{
    extern bool trash(const zz::fs::path &, zz::ec::error_code &) noexcept;
    inline bool trash(const zz::fs::path &path)
    {
        zz::ec::error_code ec;
        if (!trash(path, ec))
            throw zz::fs::filesystem_error("trash", path, ec);
        return true;
    }
}
