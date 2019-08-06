//
// Implementation from sprcache.cpp specific to AGS.Native library
//

#ifdef _MANAGED
// ensure this doesn't get compiled to .NET IL
#pragma unmanaged
#pragma warning (disable: 4996 4312)  // disable deprecation warnings
#endif

#include "util/wgt2allg.h"
#include "ac/common_defines.h"
#include "ac/gamestructdefines.h"
#include "ac/spritecache.h"

//=============================================================================
// AGS.Native-specific implementation split out of sprcache.cpp
//=============================================================================

void get_new_size_for_sprite(int ee, int ww, int hh, int &newwid, int &newhit)
{
  newwid = ww;
  newhit = hh;
}

void SpriteCache::InitNullSpriteParams(sprkey_t index)
{
    // no sprite ... blank it out
    _sprInfos[index] = SpriteInfo();
    _spriteData[index] = SpriteData();
}
