#pragma once

#ifdef _WIN32
#include "win32/dlfcn.h"
#else
#include <dlfcn.h>
#endif

#include "config.h"

#include "handle.h"

namespace zz
{
    class dll
    {
        dll(const dll &) = delete;
        dll & operator = (const dll &) = delete;
    public:
        explicit dll(const fs::path &path) noexcept
            : _handle(&dlclose)
        {
            _handle = dlopen(path.c_str(), RTLD_NOW);
        }
    protected:
        template <typename symbol_type>
        auto symbol(const char *name) const noexcept
        {
            return reinterpret_cast<symbol_type>(dlsym(_handle, name));
        }
        unique_handle<void *, decltype(&dlclose)> _handle;
    };
}
