#include <Foundation/Foundation.h>

#include "../trash.h"

static inline BOOL NSTrash(const char *path, NSError **error) noexcept
{
    auto url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path]];
    return [[NSFileManager defaultManager] trashItemAtURL:url
                                         resultingItemURL:nil
                                                    error:error];
}

bool zz::fs::trash(const zz::fs::path &path)
{
    zz::ec::error_code ec;
    if (!trash(path, ec))
        throw zz::fs::filesystem_error("trash", path, ec);
    return true;
}

bool zz::fs::trash(const zz::fs::path &path, zz::ec::error_code &ec) noexcept
{
    NSError *error = nil;
    auto succeeded = NSTrash(path.c_str(), &error);
    if (succeeded)
        ec.clear();
    else
        ec.assign(error.code, zz::ec::system_category());
    return succeeded;
}
