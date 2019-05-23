#pragma once

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "config.h"

#include "handle.h"

namespace az
{
    class dll
    {
        dll(const dll &) = delete;
        dll & operator = (const dll &) = delete;
#ifdef _WIN32
    public:
        explicit dll(const fs::path &path) noexcept
            : _handle(&::FreeLibrary)
        {
            _handle = ::LoadLibraryW(path.c_str());
        }
    protected:
        template <typename symbol_type>
        auto symbol(const char *name) const noexcept
        {
            return reinterpret_cast<symbol_type>(::GetProcAddress(_handle, name));
        }
        unique_handle<HMODULE, decltype(&::FreeLibrary)> _handle;
#else
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
#endif
    };
}
