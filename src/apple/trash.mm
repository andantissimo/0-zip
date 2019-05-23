#include <Foundation/Foundation.h>

#include "../trash.h"

static inline BOOL NSTrash(const char *path, NSError **error) noexcept
{
    auto url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path]];
    return [[NSFileManager defaultManager] trashItemAtURL:url
                                         resultingItemURL:nil
                                                    error:error];
}

bool az::fs::trash(const az::fs::path &path)
{
    az::ec::error_code ec;
    if (!trash(path, ec))
        throw az::fs::filesystem_error("trash", path, ec);
    return true;
}

bool az::fs::trash(const az::fs::path &path, az::ec::error_code &ec) noexcept
{
    NSError *error = nil;
    auto succeeded = NSTrash(path.c_str(), &error);
    if (succeeded)
        ec.clear();
    else
        ec.assign(error.code, az::ec::system_category());
    return succeeded;
}
