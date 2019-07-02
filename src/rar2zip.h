#pragma once

#include <cstdint>

#include "config.h"

#include "options.h"

namespace zz
{
    constexpr uint32_t rar_signature = 'R' | 'a' << 8 | 'r' << 16 | '!' << 24;

    bool rar_exists();

    void rar2zip(const fs::path &, const options &);
}
