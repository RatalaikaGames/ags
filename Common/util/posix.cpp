
//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

#include <string.h>
#include "posix.h"
#include "core/types.h"


//TODO - can actually include agsplatformdriver.h here.... in some cases. 
//in any platform that's not windows, actually, we're simply a game engine
//that is, the presence of the engine (and its portability facilities) can be assured
//However, we don't need this yet

//TODO - absorb stdio_compat.h

extern "C" char *_alemu_strlwr(char *string);
extern "C" char *_alemu_strupr(char *string);

int ags_stricmp(const char *string1, const char *string2)
{
    #ifdef _MSC_VER
    return _stricmp(string1, string2);
    #else
    return strcasecmp(string1, string2);
    #endif
}

int ags_strnicmp(const char *string1, const char *string2, int n)
{
    #ifdef _MSC_VER
    return _strnicmp(string1, string2, n);
    #else
    return strncasecmp(string1, string2, n);
    #endif
}

char *ags_strlwr(char *str)
{
    #ifdef _MSC_VER
    return strlwr(str);
    #else
    return _alemu_strlwr(str);
    #endif
}

char *ags_strupr(char *str)
{
    #ifdef _MSC_VER
    return strupr(str);
    #else
    return _alemu_strupr(str);
    #endif
}

int ags_unlink(const char* path)
{
    #ifdef AGS_HAS_POSIX
    return unlink(path);
    #else
    return -1;
    #endif
}