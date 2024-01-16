#include <Foundation/Foundation.h>

#include "../trash.h"

static inline BOOL NSTrash(const char *path, NSError **error) noexcept
{
    auto url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path]];
    return [[NSFileManager defaultManager] trashItemAtURL:url
                                         resultingItemURL:nil
                                                    error:error];
}

bool zz::fs::trash(const zz::fs::path &path, zz::ec::error_code &ec) noexcept
{
    NSError *error = nil;
    if (NSTrash(path.c_str(), &error)) {
        ec.clear();
        return true;
    }
    if (error.code != NSFeatureUnsupportedError) {
        ec.assign(error.code, zz::ec::system_category());
        return false;
    }

    auto backup_filename = path.filename().native() + "~";
    rename(path, path.parent_path() / backup_filename, ec);
    return !ec;
}
