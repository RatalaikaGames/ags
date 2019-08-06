
#include "core/platform.h"
#include <errno.h>
#if AGS_PLATFORM_OS_WINDOWS
#include <direct.h>
#elif defined(CONSOLE_VERSION)
#else
#include <sys/stat.h>
#include <unistd.h>
#endif
#include "util/directory.h"
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
	#ifdef CONSOLE_VERSION
	return false;
	#else
    return mkdir(path
#if ! AGS_PLATFORM_OS_WINDOWS
        , 0755
#endif
        ) == 0 || errno == EEXIST;
	#endif
}

bool CreateAllDirectories(const String &parent, const String &path)
{
    if (!ags_directory_exists(parent.GetCStr()))
        return false;
    if (path.IsEmpty())
        return true;
    if (!Path::IsSameOrSubDir(parent, path))
        return false;

    String sub_path = Path::MakeRelativePath(parent, path);
    String make_path = parent;
    std::vector<String> dirs = sub_path.Split('/');
    for (auto dir : dirs)
    {
        if (dir.IsEmpty() || dir.Compare(".") == 0) continue;
        make_path.AppendChar('/');
        make_path.Append(dir);
        if (!CreateDirectory(make_path))
            return false;
    }
    return true;
}

String SetCurrentDirectory(const String &path)
{
	#ifdef CONSOLE_VERSION
		return "";
	#else
    chdir(path);
    return GetCurrentDirectory();
	#endif
}

String GetCurrentDirectory()
{
	#ifdef CONSOLE_VERSION
		return "";
	#else
    char buf[512];
    getcwd(buf, 512);
    String str(buf);
    Path::FixupPath(str);
    return str;
	#endif

}

} // namespace Directory

} // namespace Common
} // namespace AGS
