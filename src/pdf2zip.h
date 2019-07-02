#pragma once

#include <cstdint>

#include "config.h"

#include "options.h"

namespace zz
{
    constexpr uint32_t pdf_signature = '%' | 'P' << 8 | 'D' << 16 | 'F' << 24;

    void pdf2zip(const fs::path &, const options &);
}
