#pragma once

#include <windows.h>

#define RTLD_NOW 0x2

static inline int dlclose(void *handle) noexcept
{
    return ::FreeLibrary(static_cast<HMODULE>(handle)) ? 0 : ::GetLastError();
}

static inline void * dlopen(const wchar_t *path, int /*mode*/) noexcept
{
    return ::LoadLibraryW(path);
}

static inline void * dlsym(void *handle, const char *symbol) noexcept
{
    return ::GetProcAddress(static_cast<HMODULE>(handle), symbol);
}
