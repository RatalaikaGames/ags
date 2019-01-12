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

#include "stdio.h"
#include "gfx/gfxfilter_aaconsole.h"

namespace AGS
{
namespace Engine
{
namespace AL3DConsole
{

const GfxFilterInfo AAConsoleGfxFilter::FilterInfo = GfxFilterInfo("Linear", "Linear interpolation");

const GfxFilterInfo &AAConsoleGfxFilter::GetInfo() const
{
    return FilterInfo;
}

void AAConsoleGfxFilter::SetSamplerStateForStandardSprite(void *direct3ddevice9)
{
    //d3d9->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    //d3d9->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
}

bool AAConsoleGfxFilter::NeedToColourEdgeLines()
{
    return true;
}

} // namespace AL3DConsole
} // namespace Engine
} // namespace AGS
