#include "../dll.h"
#include "../trash.h"

typedef int gboolean;

typedef uint32_t GQuark;

struct GError
{
    GQuark domain;
    int    code;
    char  *message;
};

typedef void GFile;
typedef void GCancellable;

class libgio : public zz::dll
{
public:
    explicit libgio(const zz::fs::path &path) noexcept
        : dll(path)
    {
        g_error_free        = symbol<decltype(g_error_free)>("g_error_free");
        g_object_unref      = symbol<decltype(g_object_unref)>("g_object_unref");
        g_file_new_for_path = symbol<decltype(g_file_new_for_path)>("g_file_new_for_path");
        g_file_trash        = symbol<decltype(g_file_trash)>("g_file_trash");
    }
    explicit operator bool () const noexcept
    {
        return g_error_free
            && g_object_unref
            && g_file_new_for_path
            && g_file_trash;
    }
    void     (*g_error_free)(GError *) = nullptr;
    void     (*g_object_unref)(void *) = nullptr;
    GFile *  (*g_file_new_for_path)(const char *) = nullptr;
    gboolean (*g_file_trash)(GFile *, GCancellable *, GError **) = nullptr;
};

static inline auto gio_trash(const char *path, int &code) noexcept
{
    if (libgio gio("libgio-2.0.so.0"); gio) {
        zz::unique_handle<GFile *, decltype(gio.g_object_unref)> gfile(gio.g_object_unref);
        gfile = gio.g_file_new_for_path(path);
        zz::unique_handle<GError *, decltype(gio.g_error_free)> gerror(gio.g_error_free);
        if (gio.g_file_trash(gfile, nullptr, &gerror))
            return true;
        code = static_cast<GError *>(gerror)->code;
    }
    return false;
}

bool zz::fs::trash(const zz::fs::path &path, zz::ec::error_code &ec) noexcept
{
    int code = 0;
    if (gio_trash(path.c_str(), code)) {
        ec.clear();
        return true;
    }
    if (code != 0) {
        ec.assign(code, zz::ec::system_category());
        return false;
    }

    auto backup_filename = path.filename().native() + "~";
    rename(path, path.parent_path() / backup_filename, ec);
    return !ec;
}
