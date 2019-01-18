#ifndef __AGS_CN_CORE__BUILDCONFIG_H
#define __AGS_CN_CORE__BUILDCONFIG_H

//disable features unneeded for consoles
#ifndef CONSOLE_VERSION
#define AGS_HAS_POSIX
#define AGS_HAS_RICH_GAME_MEDIA
#endif

#endif // __AGS_CN_CORE__DEFVERSION_H