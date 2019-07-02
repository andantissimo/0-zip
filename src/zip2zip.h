#pragma once

#include <cstdint>

#include "config.h"

#include "options.h"

namespace zz
{
    constexpr uint32_t zip_signature = 'P' | 'K' << 8 | 3 << 16 | 4 << 24;

    void zip2zip(const fs::path &, const options &);
}
