
#include <errno.h>
#if defined (WINDOWS_VERSION)
#include <direct.h>
#elif defined(CONSOLE_VERSION)
#else
#include <sys/stat.h>
#include <unistd.h>
#endif
#include "util/directory.h"
#include "util/path.h"

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
#if !defined (WINDOWS_VERSION)
        , 0755
#endif
        ) == 0 || errno == EEXIST;
	#endif
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
