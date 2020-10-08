
#include "core/platform.h"
#include "platform/base/agsplatformdriver.h"
#include <errno.h>
#include "util/path.h"
#include "stdio_compat.h"


namespace AGS
{
namespace Common
{

namespace Directory
{

bool CreateDirectory(const String &path)
{
    return platform->DirectoryCreate(path);
}

bool CreateAllDirectories(const String &parent, const String &path)
{
    return platform->DirectoryCreateAll(parent, path);
}

String SetCurrentDirectory(const String &path)
{
    return platform->DirectorySetCurrent(path);
}

String GetCurrentDirectory()
{
    return platform->DirectoryGetCurrent();
}

} // namespace Directory

} // namespace Common
} // namespace AGS
