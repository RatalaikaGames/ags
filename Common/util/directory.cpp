
#include <errno.h>
#if defined (WINDOWS_VERSION)
#include <direct.h>
#elif defined(AGS_RATA)
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
	#ifdef AGS_RATA
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
	#ifdef AGS_RATA
		return "";
	#else
    chdir(path);
    return GetCurrentDirectory();
	#endif
}

String GetCurrentDirectory()
{
	#ifdef AGS_RATA
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
