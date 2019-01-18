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

#ifndef __AGS_EE_UTIL__MUTEX_H
#define __AGS_EE_UTIL__MUTEX_H

namespace AGS
{
namespace Engine
{


class BaseMutex
{
public:
  BaseMutex()
  {
  };

  virtual ~BaseMutex()
  {
  };

  virtual void Lock() = 0;

  virtual void Unlock() = 0;
};


} // namespace Engine
} // namespace AGS


#if defined(WINDOWS_VERSION)
#include "mutex_windows.h"

#elif defined(PSP_VERSION)
#include "mutex_psp.h"

#elif defined(WII_VERSION)
#include "mutex_wii.h"

#elif defined(LINUX_VERSION) \
   || defined(MAC_VERSION) \
   || defined(IOS_VERSION) \
   || defined(ANDROID_VERSION)
#include "mutex_pthread.h"

#elif defined(CONSOLE_VERSION)
#include "mutex_console.h"

#endif


#endif // __AGS_EE_UTIL__MUTEX_H
