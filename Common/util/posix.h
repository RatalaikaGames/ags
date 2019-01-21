
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
//
// posix function wrappers, since we can't depend on posix availability
// (and wouldn't even want to, in case of consoles, which aren't multi-user workstations)
//
//=============================================================================
#ifndef __AGS_CN_UTIL__POSIX_H
#define __AGS_CN_UTIL__POSIX_H

//I'm sorry. This file is called "posix" yet contains "ags_" functions.
//It's not so bad.

int ags_stricmp(const char *string1, const char *string2);
int ags_strnicmp(const char *string1, const char *string2, int n);
char *ags_strlwr(char *str);
char *ags_strupr(char *str);
int ags_unlink(const char* path);

//strdup is a total disaster on microsoft compilers. let's not use it
char* ags_strdup(const char* str1);

#endif // __AGS_CN_UTIL__POSIX_H
