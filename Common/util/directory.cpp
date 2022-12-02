
#include "core/platform.h"
#include <errno.h>
#if AGS_PLATFORM_OS_WINDOWS || defined(WIN32)
#include <direct.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif
#include "util/directory.h"
#include "util/path.h"
#include "stdio_compat.h"

#include "platform/base/agsplatformdriver.h"

namespace AGS
{
namespace Common
{

namespace Directory
{

bool CreateDirectory(const String &path)
{
    //MBG - USE PLATFORM, WHY NOT????
    return platform->DirectoryCreate(path);
}

bool CreateAllDirectories(const String &parent, const String &path)
{
    if (!ags_directory_exists(parent.GetCStr()))
        return false;
    if (path.IsEmpty())
        return true;
    if (!Path::IsSameOrSubDir(parent, path))
        return false;

    String fixup_parent = Path::MakeTrailingSlash(parent);
    String sub_path = Path::MakeRelativePath(fixup_parent, path);
    String make_path = parent;
    std::vector<String> dirs = Path::Split(sub_path);
    for (const String &dir : dirs)
    {
        if (dir.IsEmpty() || dir.Compare(".") == 0) continue;
        make_path = Path::ConcatPaths(make_path, dir);
        if (!CreateDirectory(make_path))
            return false;
    }
    return true;
}

String SetCurrentDirectory(const String &path)
{
    //MBG - USE PLATFORM, WHY NOT????
    platform->DirectorySetCurrent(path);
    return GetCurrentDirectory();
}

String GetCurrentDirectory()
{
    //MBG - USE PLATFORM, WHY NOT????
    String str = platform->DirectoryGetCurrent();
    Path::FixupPath(str);
    return str;
}

} // namespace Directory

} // namespace Common
} // namespace AGS
