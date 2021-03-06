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
// Allegro Interface for 3D; Direct 3D 9 driver
//
//=============================================================================

#include <allegro.h>
#include <allegro/platform/aintwin.h>
#include "debug/assert.h"
#include "debug/out.h"
#include "gfx/ali3dexception.h"
#include "gfx/gfxfilter_d3d.h"
#include "gfx/gfxfilter_aad3d.h"
#include "gfx/gfx_util.h"
#include "main/main_allegro.h"
#include "platform/base/agsplatformdriver.h"
#include "platform/windows/gfx/ali3dd3d.h"
#include "util/library.h"

using namespace AGS::Common;

extern int dxmedia_play_video_3d(const char*filename, IDirect3DDevice9 *device, bool useAVISound, int canskip, int stretch);
extern void dxmedia_shutdown_3d();

// Necessary to update textures from 8-bit bitmaps
extern RGB palette[256];

//
// Following functions implement various matrix operations. Normally they are found in the auxiliary d3d9x.dll,
// but we do not want AGS to be dependent on it.
//
// Setup identity matrix
void MatrixIdentity(D3DMATRIX &m)
{
    m = {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    };
}
// Setup translation matrix
void MatrixTranslate(D3DMATRIX &m, float x, float y, float z)
{
    MatrixIdentity(m);
    m.m[3][0] = x;
    m.m[3][1] = y;
    m.m[3][2] = z;
}
// Setup scaling matrix
void MatrixScale(D3DMATRIX &m, float sx, float sy, float sz)
{
    MatrixIdentity(m);
    m.m[0][0] = sx;
    m.m[1][1] = sy;
    m.m[2][2] = sz;
}
// Setup rotation around Z axis; angle is in radians
void MatrixRotateZ(D3DMATRIX &m, float angle)
{
    MatrixIdentity(m);
    m.m[0][0] = cos(angle);
    m.m[1][1] = cos(angle);
    m.m[0][1] = sin(angle);
    m.m[1][0] = -sin(angle);
}
// Matrix multiplication
void MatrixMultiply(D3DMATRIX &mr, const D3DMATRIX &m1, const D3DMATRIX &m2)
{
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            mr.m[i][j] = m1.m[i][0] * m2.m[0][j] + m1.m[i][1] * m2.m[1][j] + m1.m[i][2] * m2.m[2][j] + m1.m[i][3] * m2.m[3][j];
}
// Setup full 2D transformation matrix
void MatrixTransform2D(D3DMATRIX &m, float x, float y, float sx, float sy, float anglez)
{
    D3DMATRIX translate;
    D3DMATRIX scale;
    D3DMATRIX rotate;
    MatrixTranslate(translate, x, y, 0.f);
    MatrixScale(scale, sx, sy, 1.f);
    MatrixRotateZ(rotate, anglez);

    D3DMATRIX tr1;
    MatrixMultiply(tr1, rotate, scale);
    MatrixMultiply(m, tr1, translate);
}


namespace AGS
{
namespace Engine
{
namespace D3D
{

using namespace Common;

void D3DBitmap::Dispose()
{
    if (_tiles != NULL)
    {
        for (int i = 0; i < _numTiles; i++)
            _tiles[i].texture->Release();

        free(_tiles);
        _tiles = NULL;
        _numTiles = 0;
    }
    if (_vertex != NULL)
    {
        _vertex->Release();
        _vertex = NULL;
    }
}

static D3DFORMAT color_depth_to_d3d_format(int color_depth, bool wantAlpha);
static int d3d_format_to_color_depth(D3DFORMAT format, bool secondary);

bool D3DGfxModeList::GetMode(int index, DisplayMode &mode) const
{
    if (_direct3d && index >= 0 && index < _modeCount)
    {
        D3DDISPLAYMODE d3d_mode;
        if (SUCCEEDED(_direct3d->EnumAdapterModes(D3DADAPTER_DEFAULT, _pixelFormat, index, &d3d_mode)))
        {
            mode.Width = d3d_mode.Width;
            mode.Height = d3d_mode.Height;
            mode.ColorDepth = d3d_format_to_color_depth(d3d_mode.Format, false);
            mode.RefreshRate = d3d_mode.RefreshRate;
            return true;
        }
    }
    return false;
}


void dummy_vsync() { }

#define GFX_DIRECT3D_WIN  AL_ID('D','X','3','W')
#define GFX_DIRECT3D_FULL AL_ID('D','X','3','D')

GFX_DRIVER gfx_direct3d_win =
{
   GFX_DIRECT3D_WIN,
   empty_string,
   empty_string,
   "Direct3D windowed",
   NULL,    // init
   NULL,   // exit
   NULL,                        // AL_METHOD(int, scroll, (int x, int y)); 
   dummy_vsync,   // vsync
   NULL,  // setpalette
   NULL,                        // AL_METHOD(int, request_scroll, (int x, int y));
   NULL,                        // AL_METHOD(int, poll_scroll, (void));
   NULL,                        // AL_METHOD(void, enable_triple_buffer, (void));
   NULL,  //create_video_bitmap
   NULL,  //destroy_video_bitmap
   NULL,   //show_video_bitmap
   NULL,
   NULL,  //gfx_directx_create_system_bitmap,
   NULL, //gfx_directx_destroy_system_bitmap,
   NULL, //gfx_directx_set_mouse_sprite,
   NULL, //gfx_directx_show_mouse,
   NULL, //gfx_directx_hide_mouse,
   NULL, //gfx_directx_move_mouse,
   NULL,                        // AL_METHOD(void, drawing_mode, (void));
   NULL,                        // AL_METHOD(void, save_video_state, (void*));
   NULL,                        // AL_METHOD(void, restore_video_state, (void*));
   NULL,                        // AL_METHOD(void, set_blender_mode, (int mode, int r, int g, int b, int a));
   NULL,                        // AL_METHOD(int, fetch_mode_list, (void));
   0, 0,                        // int w, h;
   FALSE,                        // int linear;
   0,                           // long bank_size;
   0,                           // long bank_gran;
   0,                           // long vid_mem;
   0,                           // long vid_phys_base;
   TRUE                         // int windowed;
};

GFX_DRIVER gfx_direct3d_full =
{
   GFX_DIRECT3D_FULL,
   empty_string,
   empty_string,
   "Direct3D fullscreen",
   NULL,    // init
   NULL,   // exit
   NULL,                        // AL_METHOD(int, scroll, (int x, int y)); 
   dummy_vsync,   // sync
   NULL,  // setpalette
   NULL,                        // AL_METHOD(int, request_scroll, (int x, int y));
   NULL,                        // AL_METHOD(int, poll_scroll, (void));
   NULL,                        // AL_METHOD(void, enable_triple_buffer, (void));
   NULL,  //create_video_bitmap
   NULL,  //destroy_video_bitmap
   NULL,   //show_video_bitmap
   NULL,
   NULL,  //gfx_directx_create_system_bitmap,
   NULL, //gfx_directx_destroy_system_bitmap,
   NULL, //gfx_directx_set_mouse_sprite,
   NULL, //gfx_directx_show_mouse,
   NULL, //gfx_directx_hide_mouse,
   NULL, //gfx_directx_move_mouse,
   NULL,                        // AL_METHOD(void, drawing_mode, (void));
   NULL,                        // AL_METHOD(void, save_video_state, (void*));
   NULL,                        // AL_METHOD(void, restore_video_state, (void*));
   NULL,                        // AL_METHOD(void, set_blender_mode, (int mode, int r, int g, int b, int a));
   NULL,                        // AL_METHOD(int, fetch_mode_list, (void));
   0, 0,                        // int w, h;
   FALSE,                        // int linear;
   0,                           // long bank_size;
   0,                           // long bank_gran;
   0,                           // long vid_mem;
   0,                           // long vid_phys_base;
   FALSE                         // int windowed;
};

// The custom FVF, which describes the custom vertex structure.
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1)

static int wnd_create_device();
// TODO: is there better way to not have this mode as a global static variable?
// wnd_create_device() is being called using wnd_call_proc, which does not take
// user parameters.
static DisplayMode d3d_mode_to_init;

D3DGraphicsDriver::D3DGraphicsDriver(IDirect3D9 *d3d) 
{
  direct3d = d3d;
  direct3ddevice = NULL;
  vertexbuffer = NULL;
  _tint_red = 0;
  _tint_green = 0;
  _tint_blue = 0;
  _screenTintLayer = NULL;
  _screenTintLayerDDB = NULL;
  _screenTintSprite.skip = true;
  _screenTintSprite.x = 0;
  _screenTintSprite.y = 0;
  pixelShader = NULL;
  _legacyPixelShader = false;
  set_up_default_vertices();
  pNativeSurface = NULL;
  availableVideoMemory = 0;
  _smoothScaling = false;
  _pixelRenderXOffset = 0;
  _pixelRenderYOffset = 0;
  _renderSprAtScreenRes = false;
  flipTypeLastTime = kFlip_None;

  // Shifts comply to D3DFMT_A8R8G8B8
  _vmem_a_shift_32 = 24;
  _vmem_r_shift_32 = 16;
  _vmem_g_shift_32 = 8;
  _vmem_b_shift_32 = 0;

  // Initialize default sprite batch, it will be used when no other batch was activated
  InitSpriteBatch(0, _spriteBatchDesc[0]);
}

void D3DGraphicsDriver::set_up_default_vertices()
{
  defaultVertices[0].position.x = 0.0f;
  defaultVertices[0].position.y = 0.0f;
  defaultVertices[0].position.z = 0.0f;
  defaultVertices[0].normal.x = 0.0f;
  defaultVertices[0].normal.y = 0.0f;
  defaultVertices[0].normal.z = -1.0f;
  //defaultVertices[0].color=0xffffffff;
  defaultVertices[0].tu=0.0;
  defaultVertices[0].tv=0.0;
  defaultVertices[1].position.x = 1.0f;
  defaultVertices[1].position.y = 0.0f;
  defaultVertices[1].position.z = 0.0f;
  defaultVertices[1].normal.x = 0.0f;
  defaultVertices[1].normal.y = 0.0f;
  defaultVertices[1].normal.z = -1.0f;
  //defaultVertices[1].color=0xffffffff;
  defaultVertices[1].tu=1.0;
  defaultVertices[1].tv=0.0;
  defaultVertices[2].position.x = 0.0f;
  defaultVertices[2].position.y = -1.0f;
  defaultVertices[2].position.z = 0.0f;
  defaultVertices[2].normal.x = 0.0f;
  defaultVertices[2].normal.y = 0.0f;
  defaultVertices[2].normal.z = -1.0f;
  //defaultVertices[2].color=0xffffffff;
  defaultVertices[2].tu=0.0;
  defaultVertices[2].tv=1.0;
  defaultVertices[3].position.x = 1.0f;
  defaultVertices[3].position.y = -1.0f;
  defaultVertices[3].position.z = 0.0f;
  defaultVertices[3].normal.x = 0.0f;
  defaultVertices[3].normal.y = 0.0f;
  defaultVertices[3].normal.z = -1.0f;
  //defaultVertices[3].color=0xffffffff;
  defaultVertices[3].tu=1.0;
  defaultVertices[3].tv=1.0;
}

void D3DGraphicsDriver::Vsync() 
{
  // do nothing on D3D
}

void D3DGraphicsDriver::OnModeSet(const DisplayMode &mode)
{
  GraphicsDriverBase::OnModeSet(mode);

  // The display mode has been set up successfully, save the
  // final refresh rate that we are using
  D3DDISPLAYMODE final_display_mode;
  if (direct3ddevice->GetDisplayMode(0, &final_display_mode) == D3D_OK) {
    _mode.RefreshRate = final_display_mode.RefreshRate;
  }
  else {
    _mode.RefreshRate = 0;
  }
}

void D3DGraphicsDriver::ReleaseDisplayMode()
{
  if (!IsModeSet())
    return;

  OnModeReleased();

  ClearDrawLists();
  ClearDrawBackups();
  flipTypeLastTime = kFlip_None;

  if (_screenTintLayerDDB != NULL) 
  {
    this->DestroyDDB(_screenTintLayerDDB);
    _screenTintLayerDDB = NULL;
    _screenTintSprite.bitmap = NULL;
  }
  delete _screenTintLayer;
  _screenTintLayer = NULL;

  DestroyAllStageScreens();

  gfx_driver = NULL;

  if (platform->ExitFullscreenMode())
    platform->RestoreWindowStyle();
}

int D3DGraphicsDriver::FirstTimeInit()
{
  HRESULT hr;

  direct3ddevice->GetDeviceCaps(&direct3ddevicecaps);

  // the PixelShader.fx uses ps_1_4
  // the PixelShaderLegacy.fx needs ps_2_0
  int requiredPSMajorVersion = 1;
  int requiredPSMinorVersion = 4;
  if (_legacyPixelShader) {
    requiredPSMajorVersion = 2;
    requiredPSMinorVersion = 0;
  }

  if (direct3ddevicecaps.PixelShaderVersion < D3DPS_VERSION(requiredPSMajorVersion, requiredPSMinorVersion))  
  {
    direct3ddevice->Release();
    direct3ddevice = NULL;
    previousError = 
        set_allegro_error("Graphics card does not support Pixel Shader %d.%d", requiredPSMajorVersion, requiredPSMinorVersion);
    return -1;
  }

  // Load the pixel shader!!
  HMODULE exeHandle = GetModuleHandle(NULL);
  HRSRC hRes = FindResource(exeHandle, (_legacyPixelShader) ? "PIXEL_SHADER_LEGACY" : "PIXEL_SHADER", "DATA");
  if (hRes)
  {
    HGLOBAL hGlobal = LoadResource(exeHandle, hRes);
    if (hGlobal)
    {
      DWORD resourceSize = SizeofResource(exeHandle, hRes);
      DWORD *dataPtr = (DWORD*)LockResource(hGlobal);
      hr = direct3ddevice->CreatePixelShader(dataPtr, &pixelShader);
      if (hr != D3D_OK)
      {
        direct3ddevice->Release();
        direct3ddevice = NULL;
        previousError = set_allegro_error("Failed to create pixel shader: 0x%08X", hr);
        return -1;
      }
      UnlockResource(hGlobal);
    }
  }
  
  if (pixelShader == NULL)
  {
    direct3ddevice->Release();
    direct3ddevice = NULL;
    previousError = set_allegro_error("Failed to load pixel shader resource");
    return -1;
  }

  if (direct3ddevice->CreateVertexBuffer(4*sizeof(CUSTOMVERTEX), D3DUSAGE_WRITEONLY,
          D3DFVF_CUSTOMVERTEX, D3DPOOL_MANAGED, &vertexbuffer, NULL) != D3D_OK) 
  {
    direct3ddevice->Release();
    direct3ddevice = NULL;
    previousError = set_allegro_error("Failed to create vertex buffer");
    return -1;
  }

  // This line crashes because my card doesn't support 8-bit textures
  //direct3ddevice->SetCurrentTexturePalette(0);

  CUSTOMVERTEX *vertices;
  vertexbuffer->Lock(0, 0, (void**)&vertices, D3DLOCK_DISCARD);

  for (int i = 0; i < 4; i++)
  {
    vertices[i] = defaultVertices[i];
  }

  vertexbuffer->Unlock();

  direct3ddevice->GetGammaRamp(0, &defaultgammaramp);

  if (defaultgammaramp.red[255] < 256)
  {
    // correct bug in some gfx drivers that returns gamma ramp
    // values from 0-255 instead of 0-65535
    for (int i = 0; i < 256; i++)
    {
      defaultgammaramp.red[i] *= 256;
      defaultgammaramp.green[i] *= 256;
      defaultgammaramp.blue[i] *= 256;
    }
  }
  currentgammaramp = defaultgammaramp;

  return 0;
}

void D3DGraphicsDriver::initD3DDLL(const DisplayMode &mode) 
{
   if (!IsModeSupported(mode))
   {
     throw Ali3DException(get_allegro_error());
   }

   _enter_critical();

   d3d_mode_to_init = mode;
   // Set the display mode in the window's thread
   if (wnd_call_proc(wnd_create_device)) {
     _exit_critical();
     throw Ali3DException(get_allegro_error());
   }

   availableVideoMemory = direct3ddevice->GetAvailableTextureMem();

   _exit_critical();

   // Set up a fake allegro gfx driver so that things like
   // the allegro mouse handler still work
   if (mode.Windowed)
     gfx_driver = &gfx_direct3d_win;
   else
     gfx_driver = &gfx_direct3d_full;

   return;
}

/* color_depth_to_d3d_format:
 *  Convert a colour depth into the appropriate D3D tag
 */
static D3DFORMAT color_depth_to_d3d_format(int color_depth, bool wantAlpha)
{
  if (wantAlpha)
  {
    switch (color_depth)
    {
      case 8:
        return D3DFMT_P8;
      case 15:
      case 16:
        return D3DFMT_A1R5G5B5;
      case 24:
      case 32:
        return D3DFMT_A8R8G8B8;
    }
  }
  else
  {
    switch (color_depth)
    {
      case 8:
        return D3DFMT_P8;
      case 15:  // don't use X1R5G5B5 because some cards don't support it
        return D3DFMT_A1R5G5B5;
      case 16:
        return D3DFMT_R5G6B5;
      case 24:
        return D3DFMT_R8G8B8;
      case 32:
        return D3DFMT_X8R8G8B8;
    }
  }
  return D3DFMT_UNKNOWN;
}

/* d3d_format_to_color_depth:
 *  Convert a D3D tag to colour depth
 *
 * TODO: this is currently an inversion of color_depth_to_d3d_format;
 * check later if more formats should be handled
 */
static int d3d_format_to_color_depth(D3DFORMAT format, bool secondary)
{
  switch (format)
  {
  case D3DFMT_P8:
    return 8;
  case D3DFMT_A1R5G5B5:
    return secondary ? 15 : 16;
  case D3DFMT_X1R5G5B5:
    return secondary ? 15 : 16;
  case D3DFMT_R5G6B5:
    return 16;
  case D3DFMT_R8G8B8:
    return secondary ? 24 : 32;
  case D3DFMT_A8R8G8B8:
  case D3DFMT_X8R8G8B8:
    return 32;
  }
  return 0;
}

bool D3DGraphicsDriver::IsModeSupported(const DisplayMode &mode)
{
  if (mode.Width <= 0 || mode.Height <= 0 || mode.ColorDepth <= 0)
  {
    set_allegro_error("Invalid resolution parameters: %d x %d x %d", mode.Width, mode.Height, mode.ColorDepth);
    return false;
  }

  if (mode.Windowed)
  {
    return true;
  }

  D3DFORMAT pixelFormat = color_depth_to_d3d_format(mode.ColorDepth, false);
  D3DDISPLAYMODE d3d_mode;

  int mode_count = direct3d->GetAdapterModeCount(D3DADAPTER_DEFAULT, pixelFormat);
  for (int i = 0; i < mode_count; i++)
  {
    if (FAILED(direct3d->EnumAdapterModes(D3DADAPTER_DEFAULT, pixelFormat, i, &d3d_mode)))
    {
      set_allegro_error("IDirect3D9::EnumAdapterModes failed");
      return false;
    }

    if ((d3d_mode.Width == mode.Width) && (d3d_mode.Height == mode.Height))
    {
      return true;
    }
  }

  set_allegro_error("The requested adapter mode is not supported");
  return false;
}

bool D3DGraphicsDriver::SupportsGammaControl() 
{
  if ((direct3ddevicecaps.Caps2 & D3DCAPS2_FULLSCREENGAMMA) == 0)
    return false;

  if (_mode.Windowed)
    return false;

  return true;
}

void D3DGraphicsDriver::SetGamma(int newGamma)
{
  for (int i = 0; i < 256; i++) 
  {
    int newValue = ((int)defaultgammaramp.red[i] * newGamma) / 100;
    if (newValue >= 65535)
      newValue = 65535;
    currentgammaramp.red[i] = newValue;
    currentgammaramp.green[i] = newValue;
    currentgammaramp.blue[i] = newValue;
  }

  direct3ddevice->SetGammaRamp(0, D3DSGR_NO_CALIBRATION, &currentgammaramp);
}

/* wnd_set_video_mode:
 *  Called by window thread to set a gfx mode; this is needed because DirectDraw can only
 *  change the mode in the thread that handles the window.
 */
static int wnd_create_device()
{
  return D3DGraphicsFactory::GetD3DDriver()->_initDLLCallback(d3d_mode_to_init);
}

static int wnd_reset_device()
{
  return D3DGraphicsFactory::GetD3DDriver()->_resetDeviceIfNecessary();
}

int D3DGraphicsDriver::_resetDeviceIfNecessary()
{
  HRESULT hr = direct3ddevice->TestCooperativeLevel();

  if (hr == D3DERR_DEVICELOST)
  {
    Debug::Printf("D3DGraphicsDriver: D3D Device Lost");
    // user has alt+tabbed away from the game
    return 1;
  }

  if (hr == D3DERR_DEVICENOTRESET)
  {
    Debug::Printf("D3DGraphicsDriver: D3D Device Not Reset");
    hr = ResetD3DDevice();
    if (hr != D3D_OK)
    {
      Debug::Printf("D3DGraphicsDriver: Failed to reset D3D device");
      // can't throw exception because we're in the wrong thread,
      // so just return a value instead
      return 2;
    }

    InitializeD3DState();
    CreateVirtualScreen();
    direct3ddevice->SetGammaRamp(0, D3DSGR_NO_CALIBRATION, &currentgammaramp);
  }

  return 0;
}

int D3DGraphicsDriver::_initDLLCallback(const DisplayMode &mode)
{
  HWND allegro_wnd = win_get_window();

  if (!mode.Windowed)
  {
    if (platform->EnterFullscreenMode(mode))
      platform->AdjustWindowStyleForFullscreen();
  }

  memset( &d3dpp, 0, sizeof(d3dpp) );
  d3dpp.BackBufferWidth = mode.Width;
  d3dpp.BackBufferHeight = mode.Height;
  d3dpp.BackBufferFormat = color_depth_to_d3d_format(mode.ColorDepth, false);
  d3dpp.BackBufferCount = 1;
  d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
  // THIS MUST BE SWAPEFFECT_COPY FOR PlayVideo TO WORK
  d3dpp.SwapEffect = D3DSWAPEFFECT_COPY; //D3DSWAPEFFECT_DISCARD; 
  d3dpp.hDeviceWindow = allegro_wnd;
  d3dpp.Windowed = mode.Windowed;
  d3dpp.EnableAutoDepthStencil = FALSE;
  d3dpp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER; // we need this flag to access the backbuffer with lockrect
  d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
  if(mode.Vsync)
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
  else
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
  /* If full screen, specify the refresh rate */
  if ((d3dpp.Windowed == FALSE) && (mode.RefreshRate > 0))
    d3dpp.FullScreen_RefreshRateInHz = mode.RefreshRate;

  if (_initGfxCallback != NULL)
    _initGfxCallback(&d3dpp);

  bool first_time_init = direct3ddevice == NULL;
  HRESULT hr = 0;
  if (direct3ddevice)
  {
    hr = ResetD3DDevice();
  }
  else
    hr = direct3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, allegro_wnd,
                      D3DCREATE_MIXED_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,  // multithreaded required for AVI player
                      &d3dpp, &direct3ddevice);
  if (hr != D3D_OK)
  {
    if (!previousError.IsEmpty())
      set_allegro_error(previousError);
    else
      set_allegro_error("Failed to create Direct3D Device: 0x%08X", hr);
    return -1;
  }

  if (mode.Windowed)
  {
    if (adjust_window(mode.Width, mode.Height) != 0) 
    {
      direct3ddevice->Release();
      direct3ddevice = NULL;
      set_allegro_error("Window size not supported");
      return -1;
    }
  }

  win_grab_input();

  if (first_time_init)
  {
    int ft_res = FirstTimeInit();
    if (ft_res != 0)
      return ft_res;
  }
  return 0;
}

void D3DGraphicsDriver::InitializeD3DState()
{
  direct3ddevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_RGBA(30, 0, 0, 255), 0.5f, 0);

  // set the render flags.
  direct3ddevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

  direct3ddevice->SetRenderState(D3DRS_LIGHTING, true);
  direct3ddevice->SetRenderState(D3DRS_ZENABLE, FALSE);

  direct3ddevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  direct3ddevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
  direct3ddevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

  direct3ddevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
  direct3ddevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER); 
  direct3ddevice->SetRenderState(D3DRS_ALPHAREF, (DWORD)0);

  direct3ddevice->SetFVF(D3DFVF_CUSTOMVERTEX);

  D3DMATERIAL9 material;
  ZeroMemory(&material, sizeof(material));				//zero memory ( NEW )
  material.Diffuse.r = 1.0f;						//diffuse color ( NEW )
  material.Diffuse.g = 1.0f;
  material.Diffuse.b = 1.0f;
  direct3ddevice->SetMaterial(&material);

  D3DLIGHT9 d3dLight;
  ZeroMemory(&d3dLight, sizeof(D3DLIGHT9));
      
  // Set up a white point light.
  d3dLight.Type = D3DLIGHT_DIRECTIONAL;
  d3dLight.Diffuse.r  = 1.0f;
  d3dLight.Diffuse.g  = 1.0f;
  d3dLight.Diffuse.b  = 1.0f;
  d3dLight.Diffuse.a  = 1.0f;
  d3dLight.Ambient.r  = 1.0f;
  d3dLight.Ambient.g  = 1.0f;
  d3dLight.Ambient.b  = 1.0f;
  d3dLight.Specular.r = 1.0f;
  d3dLight.Specular.g = 1.0f;
  d3dLight.Specular.b = 1.0f;
      
  // Position it high in the scene and behind the user.
  // Remember, these coordinates are in world space, so
  // the user could be anywhere in world space, too. 
  // For the purposes of this example, assume the user
  // is at the origin of world space.
  d3dLight.Direction.x = 0.0f;
  d3dLight.Direction.y = 0.0f;
  d3dLight.Direction.z = 1.0f;
      
  // Don't attenuate.
  d3dLight.Attenuation0 = 1.0f; 
  d3dLight.Range        = 1000.0f;
      
  // Set the property information for the first light.
  direct3ddevice->SetLight(0, &d3dLight);
  direct3ddevice->LightEnable(0, TRUE);

  // If we already have a render frame configured, then setup viewport immediately
  SetupViewport();
}

void D3DGraphicsDriver::SetupViewport()
{
  if (!IsModeSet() || !IsRenderFrameValid() || !IsNativeSizeValid())
    return;

  // Setup orthographic projection matrix
  D3DMATRIX matOrtho = {
    (2.0 / (float)_srcRect.GetWidth()), 0.0, 0.0, 0.0,
    0.0, (2.0 / (float)_srcRect.GetHeight()), 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 1.0
  };

  D3DMATRIX matIdentity = {
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0
  };

  direct3ddevice->SetTransform(D3DTS_PROJECTION, &matOrtho);
  direct3ddevice->SetTransform(D3DTS_WORLD, &matIdentity);
  direct3ddevice->SetTransform(D3DTS_VIEW, &matIdentity);

  // See "Directly Mapping Texels to Pixels" MSDN article for why this is necessary
  // http://msdn.microsoft.com/en-us/library/windows/desktop/bb219690.aspx
  _pixelRenderXOffset = ((float)_srcRect.GetWidth() / (float)_mode.Width) / 2.0f;
  _pixelRenderYOffset = ((float)_srcRect.GetHeight() / (float)_mode.Height) / 2.0f;

  // Clear the screen before setting a viewport.
  ClearRectangle(0, 0, _mode.Width, _mode.Height, 0);

  // Set Viewport.
  D3DVIEWPORT9 d3dViewport;
  ZeroMemory(&d3dViewport, sizeof(D3DVIEWPORT9));
  d3dViewport.X = _dstRect.Left;
  d3dViewport.Y = _dstRect.Top;
  d3dViewport.Width = _dstRect.GetWidth();
  d3dViewport.Height = _dstRect.GetHeight();
  d3dViewport.MinZ = 0.0f;
  d3dViewport.MaxZ = 1.0f;
  direct3ddevice->SetViewport(&d3dViewport);

  viewport_rect.left   = _dstRect.Left;
  viewport_rect.right  = _dstRect.Right + 1;
  viewport_rect.top    = _dstRect.Top;
  viewport_rect.bottom = _dstRect.Bottom + 1;
}

void D3DGraphicsDriver::SetGraphicsFilter(PD3DFilter filter)
{
  _filter = filter;
  OnSetFilter();
  // Creating ddbs references filter properties at some point,
  // so we have to redo this part of initialization here.
  create_screen_tint_bitmap();
}

void D3DGraphicsDriver::SetTintMethod(TintMethod method) 
{
  _legacyPixelShader = (method == TintReColourise);
}

bool D3DGraphicsDriver::SetDisplayMode(const DisplayMode &mode, volatile int *loopTimer)
{
  ReleaseDisplayMode();

  if (mode.ColorDepth < 15)
  {
    set_allegro_error("Direct3D driver does not support 256-color display mode");
    return false;
  }

  try
  {
    initD3DDLL(mode);
  }
  catch (Ali3DException exception)
  {
    if (exception._message != get_allegro_error())
      set_allegro_error(exception._message);
    return false;
  }
  OnInit(loopTimer);
  OnModeSet(mode);
  InitializeD3DState();
  CreateVirtualScreen();
  create_screen_tint_bitmap();
  return true;
}

void D3DGraphicsDriver::CreateVirtualScreen()
{
  if (!IsModeSet() || !IsNativeSizeValid())
    return;

  // set up native surface
  if (pNativeSurface != NULL)
  {
    pNativeSurface->Release();
    pNativeSurface = NULL;
  }
  if (direct3ddevice->CreateRenderTarget(
          _srcRect.GetWidth(),
          _srcRect.GetHeight(),
          color_depth_to_d3d_format(_mode.ColorDepth, false),
          D3DMULTISAMPLE_NONE,
          0,
          true,
          &pNativeSurface,
          NULL  )!= D3D_OK)
  {
    throw Ali3DException("CreateRenderTarget failed");
  }
  direct3ddevice->ColorFill(pNativeSurface, NULL, 0);

  // create initial stage screen for plugin raw drawing
  _stageVirtualScreen = CreateStageScreen(0, _srcRect.GetSize());
  BitmapHelper::SetScreenBitmap(_stageVirtualScreen.get());
}

HRESULT D3DGraphicsDriver::ResetD3DDevice()
{
  // Direct3D documentation:
  // Before calling the IDirect3DDevice9::Reset method for a device,
  // an application should release any explicit render targets, depth stencil
  // surfaces, additional swap chains, state blocks, and D3DPOOL_DEFAULT
  // resources associated with the device.
  if (pNativeSurface != NULL)
  {
    pNativeSurface->Release();
    pNativeSurface = NULL;
  }

  return direct3ddevice->Reset(&d3dpp);
}

bool D3DGraphicsDriver::SetNativeSize(const Size &src_size)
{
  OnSetNativeSize(src_size);
  // Also make sure viewport is updated using new native & destination rectangles
  SetupViewport();
  CreateVirtualScreen();
  return !_srcRect.IsEmpty();
}

bool D3DGraphicsDriver::SetRenderFrame(const Rect &dst_rect)
{
  OnSetRenderFrame(dst_rect);
  // Also make sure viewport is updated using new native & destination rectangles
  SetupViewport();
  return !_dstRect.IsEmpty();
}

int D3DGraphicsDriver::GetDisplayDepthForNativeDepth(int native_color_depth) const
{
    // TODO: check for device caps to know which depth is supported?
    return 32;
}

IGfxModeList *D3DGraphicsDriver::GetSupportedModeList(int color_depth)
{
  direct3d->AddRef();
  return new D3DGfxModeList(direct3d, color_depth_to_d3d_format(color_depth, false));
}

PGfxFilter D3DGraphicsDriver::GetGraphicsFilter() const
{
    return _filter;
}

void D3DGraphicsDriver::UnInit() 
{
  OnUnInit();
  ReleaseDisplayMode();

  dxmedia_shutdown_3d();

  if (pNativeSurface)
  {
    pNativeSurface->Release();
    pNativeSurface = NULL;
  }

  if (vertexbuffer)
  {
    vertexbuffer->Release();
    vertexbuffer = NULL;
  }

  if (pixelShader)
  {
    pixelShader->Release();
    pixelShader = NULL;
  }

  if (direct3ddevice)
  {
    direct3ddevice->Release();
    direct3ddevice = NULL;
  }
}

D3DGraphicsDriver::~D3DGraphicsDriver()
{
  UnInit();

  if (direct3d)
    direct3d->Release();
}

void D3DGraphicsDriver::ClearRectangle(int x1, int y1, int x2, int y2, RGB *colorToUse)
{
  // NOTE: this function is practically useless at the moment, because D3D redraws whole game frame each time
  Rect r(x1, y1, x2, y2);
  r = _scaling.ScaleRange(r);
  D3DRECT rectToClear;
  rectToClear.x1 = r.Left;
  rectToClear.y1 = r.Top;
  rectToClear.x2 = r.Right + 1;
  rectToClear.y2 = r.Bottom + 1;
  DWORD colorDword = 0;
  if (colorToUse != NULL)
    colorDword = D3DCOLOR_XRGB(colorToUse->r, colorToUse->g, colorToUse->b);

  direct3ddevice->Clear(1, &rectToClear, D3DCLEAR_TARGET, colorDword, 0.5f, 0);
}

void D3DGraphicsDriver::GetCopyOfScreenIntoBitmap(Bitmap *destination, bool at_native_res)
{
  D3DDISPLAYMODE displayMode;
  direct3ddevice->GetDisplayMode(0, &displayMode);

  // If we are rendering sprites at the screen resolution, and requested native res,
  // re-render last frame to the native surface
  if (at_native_res && _renderSprAtScreenRes)
  {
    _renderSprAtScreenRes = false;
    _reDrawLastFrame();
    _render(flipTypeLastTime, true);
    _renderSprAtScreenRes = true;
  }
  else if (!_renderSprAtScreenRes)
  {
    at_native_res = true;
  }
  
  IDirect3DSurface9* surface = NULL;
  {
    if (_pollingCallback)
      _pollingCallback();

    if (at_native_res)
    {
      surface = pNativeSurface;
    }
    // Get the back buffer surface
    else if (direct3ddevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surface) != D3D_OK)
    {
      throw Ali3DException("IDirect3DDevice9::GetBackBuffer failed");
    }

    if (_pollingCallback)
      _pollingCallback();

    Bitmap *retrieveInto = destination;
    if (at_native_res)
    {
        retrieveInto = BitmapHelper::CreateBitmap(_srcRect.GetWidth(), _srcRect.GetHeight(), d3d_format_to_color_depth(displayMode.Format, false));
    }
    else if ((_srcRect.GetWidth() != _dstRect.GetWidth()) ||
        (_srcRect.GetHeight() != _dstRect.GetHeight()))
    {
        retrieveInto = BitmapHelper::CreateBitmap(_dstRect.GetWidth(), _dstRect.GetHeight(), _mode.ColorDepth);
    }

    D3DLOCKED_RECT lockedRect;
    if (surface->LockRect(&lockedRect, (at_native_res ? NULL : &viewport_rect), D3DLOCK_READONLY ) != D3D_OK)
    {
      throw Ali3DException("IDirect3DSurface9::LockRect failed");
    }

    BitmapHelper::ReadPixelsFromMemory(retrieveInto, (uint8_t*)lockedRect.pBits, lockedRect.Pitch);

    surface->UnlockRect();
    if (!at_native_res) // native surface is released elsewhere
        surface->Release();

    if (_pollingCallback)
      _pollingCallback();

    if (retrieveInto != destination)
    {
      destination->StretchBlt(retrieveInto, RectWH(0, 0, retrieveInto->GetWidth(), retrieveInto->GetHeight()),
                   RectWH(0, 0, destination->GetWidth(), destination->GetHeight()));
      delete retrieveInto;

      if (_pollingCallback)
        _pollingCallback();
    }
  }
}

void D3DGraphicsDriver::RenderToBackBuffer()
{
  throw Ali3DException("D3D driver does not have a back buffer");
}

void D3DGraphicsDriver::Render()
{
  Render(kFlip_None);
}

void D3DGraphicsDriver::Render(GlobalFlipType flip)
{
  if (wnd_call_proc(wnd_reset_device))
  {
    throw Ali3DFullscreenLostException();
  }

  _renderAndPresent(flip, true);
}

void D3DGraphicsDriver::_reDrawLastFrame()
{
  RestoreDrawLists();
}

void D3DGraphicsDriver::_renderSprite(const D3DDrawListEntry *drawListEntry, const D3DMATRIX &matGlobal, bool globalLeftRightFlip, bool globalTopBottomFlip)
{
  HRESULT hr;
  D3DBitmap *bmpToDraw = drawListEntry->bitmap;
  D3DMATRIX matSelfTransform;
  D3DMATRIX matTransform;

  if (bmpToDraw->_transparency >= 255)
    return;

  if (bmpToDraw->_tintSaturation > 0)
  {
    // Use custom pixel shader
    float vector[8];
    if (_legacyPixelShader)
    {
      rgb_to_hsv(bmpToDraw->_red, bmpToDraw->_green, bmpToDraw->_blue, &vector[0], &vector[1], &vector[2]);
      vector[0] /= 360.0; // In HSV, Hue is 0-360
    }
    else
    {
      vector[0] = (float)bmpToDraw->_red / 256.0;
      vector[1] = (float)bmpToDraw->_green / 256.0;
      vector[2] = (float)bmpToDraw->_blue / 256.0;
    }

    vector[3] = (float)bmpToDraw->_tintSaturation / 256.0;

    if (bmpToDraw->_transparency > 0)
      vector[4] = (float)bmpToDraw->_transparency / 256.0;
    else
      vector[4] = 1.0f;

    if (bmpToDraw->_lightLevel > 0)
      vector[5] = (float)bmpToDraw->_lightLevel / 256.0;
    else
      vector[5] = 1.0f;

    direct3ddevice->SetPixelShaderConstantF(0, &vector[0], 2);
    direct3ddevice->SetPixelShader(pixelShader);
  }
  else
  {
    // Not using custom pixel shader; set up the default one
    direct3ddevice->SetPixelShader(NULL);
    int useTintRed = 255;
    int useTintGreen = 255;
    int useTintBlue = 255;
    int useTransparency = 0xff;
    int textureColorOp = D3DTOP_MODULATE;

    if ((bmpToDraw->_lightLevel > 0) && (bmpToDraw->_lightLevel < 256))
    {
      // darkening the sprite... this stupid calculation is for
      // consistency with the allegro software-mode code that does
      // a trans blend with a (8,8,8) sprite
      useTintRed = (bmpToDraw->_lightLevel * 192) / 256 + 64;
      useTintGreen = useTintRed;
      useTintBlue = useTintRed;
    }
    else if (bmpToDraw->_lightLevel > 256)
    {
      // ideally we would use a multi-stage operation here
      // because we need to do TEXTURE + (TEXTURE x LIGHT)
      // but is it worth having to set the device to 2-stage?
      textureColorOp = D3DTOP_ADD;
      useTintRed = (bmpToDraw->_lightLevel - 256) / 2;
      useTintGreen = useTintRed;
      useTintBlue = useTintRed;
    }

    if (bmpToDraw->_transparency > 0)
    {
      useTransparency = bmpToDraw->_transparency;
    }

    direct3ddevice->SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_RGBA(useTintRed, useTintGreen, useTintBlue, useTransparency));
    direct3ddevice->SetTextureStageState(0, D3DTSS_COLOROP, textureColorOp);
    direct3ddevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    direct3ddevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);

    if (bmpToDraw->_transparency == 0)
    {
      // No transparency, use texture alpha component
      direct3ddevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
      direct3ddevice->SetTextureStageState(0, D3DTSS_ALPHAARG1,  D3DTA_TEXTURE);
    }
    else
    {
      // Fixed transparency, use (TextureAlpha x FixedTranslucency)
      direct3ddevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
      direct3ddevice->SetTextureStageState(0, D3DTSS_ALPHAARG1,  D3DTA_TEXTURE);
      direct3ddevice->SetTextureStageState(0, D3DTSS_ALPHAARG2,  D3DTA_TFACTOR);
    }
  }

  if (bmpToDraw->_vertex == NULL)
  {
    hr = direct3ddevice->SetStreamSource(0, vertexbuffer, 0, sizeof(CUSTOMVERTEX));
  }
  else
  {
    hr = direct3ddevice->SetStreamSource(0, bmpToDraw->_vertex, 0, sizeof(CUSTOMVERTEX));
  }
  if (hr != D3D_OK) 
  {
    throw Ali3DException("IDirect3DDevice9::SetStreamSource failed");
  }

  float width = bmpToDraw->GetWidthToRender();
  float height = bmpToDraw->GetHeightToRender();
  float xProportion = width / (float)bmpToDraw->_width;
  float yProportion = height / (float)bmpToDraw->_height;

  bool  flipLeftToRight = globalLeftRightFlip ^ bmpToDraw->_flipped;
  float drawAtX = drawListEntry->x + _globalViewOff.X;
  float drawAtY = drawListEntry->y + _globalViewOff.Y;

  for (int ti = 0; ti < bmpToDraw->_numTiles; ti++)
  {
    width = bmpToDraw->_tiles[ti].width * xProportion;
    height = bmpToDraw->_tiles[ti].height * yProportion;
    float xOffs;
    float yOffs = bmpToDraw->_tiles[ti].y * yProportion;
    if (flipLeftToRight != globalLeftRightFlip)
    {
      xOffs = (bmpToDraw->_width - (bmpToDraw->_tiles[ti].x + bmpToDraw->_tiles[ti].width)) * xProportion;
    }
    else
    {
      xOffs = bmpToDraw->_tiles[ti].x * xProportion;
    }
    float thisX = drawAtX + xOffs;
    float thisY = drawAtY + yOffs;

    if (globalLeftRightFlip)
    {
      thisX = (_srcRect.GetWidth() - thisX) - width;
    }
    if (globalTopBottomFlip) 
    {
      thisY = (_srcRect.GetHeight() - thisY) - height;
    }

    thisX = (-(_srcRect.GetWidth() / 2)) + thisX;
    thisY = (_srcRect.GetHeight() / 2) - thisY;

    //Setup translation and scaling matrices
    float widthToScale = (float)width;
    float heightToScale = (float)height;
    if (flipLeftToRight)
    {
      // The usual transform changes 0..1 into 0..width
      // So first negate it (which changes 0..w into -w..0)
      widthToScale = -widthToScale;
      // and now shift it over to make it 0..w again
      thisX += width;
    }
    if (globalTopBottomFlip) 
    {
      heightToScale = -heightToScale;
      thisY -= height;
    }

    // Multiply object's own and global matrixes
    MatrixTransform2D(matSelfTransform, (float)thisX - _pixelRenderXOffset, (float)thisY + _pixelRenderYOffset, widthToScale, heightToScale, 0.f);
    MatrixMultiply(matTransform, matSelfTransform, matGlobal);

    if ((_smoothScaling) && bmpToDraw->_useResampler && (bmpToDraw->_stretchToHeight > 0) &&
        ((bmpToDraw->_stretchToHeight != bmpToDraw->_height) ||
         (bmpToDraw->_stretchToWidth != bmpToDraw->_width)))
    {
      direct3ddevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
      direct3ddevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    }
    else if (!_renderSprAtScreenRes)
    {
      direct3ddevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
      direct3ddevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    }
    else
    {
      _filter->SetSamplerStateForStandardSprite(direct3ddevice);
    }

    direct3ddevice->SetTransform(D3DTS_WORLD, &matTransform);
    direct3ddevice->SetTexture(0, bmpToDraw->_tiles[ti].texture);

    hr = direct3ddevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, ti * 4, 2);
    if (hr != D3D_OK) 
    {
      throw Ali3DException("IDirect3DDevice9::DrawPrimitive failed");
    }

  }
}

void D3DGraphicsDriver::_renderAndPresent(GlobalFlipType flip, bool clearDrawListAfterwards)
{
  _render(flip, clearDrawListAfterwards);
  direct3ddevice->Present(NULL, NULL, NULL, NULL);
}

void D3DGraphicsDriver::_render(GlobalFlipType flip, bool clearDrawListAfterwards)
{
  IDirect3DSurface9 *pBackBuffer = NULL;

  D3DVIEWPORT9 pViewport;

  if (!_renderSprAtScreenRes) {
    direct3ddevice->GetViewport(&pViewport);

    if (direct3ddevice->GetRenderTarget(0, &pBackBuffer) != D3D_OK)
    {
      throw Ali3DException("IDirect3DSurface9::GetRenderTarget failed");
    }
    if (direct3ddevice->SetRenderTarget(0, pNativeSurface) != D3D_OK)
    {
      throw Ali3DException("IDirect3DSurface9::SetRenderTarget failed");
    }
  }

  direct3ddevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_RGBA(0, 0, 0, 128), 0.5f, 0);
  if (direct3ddevice->BeginScene() != D3D_OK)
    throw Ali3DException("IDirect3DDevice9::BeginScene failed");

  // if showing at 2x size, the sprite can get distorted otherwise
  direct3ddevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
  direct3ddevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
  direct3ddevice->SetSamplerState(0, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);

  RenderSpriteBatches(flip);

  direct3ddevice->EndScene();

  if (!_renderSprAtScreenRes) {
    if (direct3ddevice->SetRenderTarget(0, pBackBuffer)!= D3D_OK)
    {
      throw Ali3DException("IDirect3DSurface9::SetRenderTarget failed");
    }
    // use correct sampling method when stretching buffer to the final rect
    _filter->SetSamplerStateForStandardSprite(direct3ddevice);
    D3DTEXTUREFILTERTYPE filterType;
    direct3ddevice->GetSamplerState(0, D3DSAMP_MAGFILTER, (DWORD*)&filterType);
    if (direct3ddevice->StretchRect(pNativeSurface, NULL, pBackBuffer, &viewport_rect, filterType) != D3D_OK)
    {
      throw Ali3DException("IDirect3DSurface9::StretchRect failed");
    }
    direct3ddevice->SetViewport(&pViewport);
  }

  if (!_renderSprAtScreenRes) {
    pBackBuffer->Release();
  }

  if (clearDrawListAfterwards)
  {
    BackupDrawLists();
    flipTypeLastTime = flip;
    ClearDrawLists();
  }
}

void D3DGraphicsDriver::RenderSpriteBatches(GlobalFlipType flip)
{
    // Render all the sprite batches with necessary transformations
    for (size_t i = 0; i <= _actSpriteBatch; ++i)
    {
        const Rect &viewport = _spriteBatchDesc[i].Viewport;
        const D3DSpriteBatch &batch = _spriteBatches[i];
        if (!viewport.IsEmpty())
        {
            direct3ddevice->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
            RECT scissor;
            if (_renderSprAtScreenRes)
            {
                scissor.left = _scaling.X.ScalePt(viewport.Left);
                scissor.top = _scaling.Y.ScalePt(viewport.Top);
                scissor.right = _scaling.X.ScalePt(viewport.Right + 1);
                scissor.bottom = _scaling.Y.ScalePt(viewport.Bottom + 1);
            }
            else
            {
                scissor.left = viewport.Left;
                scissor.top = viewport.Top;
                scissor.right = viewport.Right + 1;
                scissor.bottom = viewport.Bottom + 1;
            }
            direct3ddevice->SetScissorRect(&scissor);
        }
        else
        {
            direct3ddevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        }
        _stageVirtualScreen = GetStageScreen(i);
        RenderSpriteBatch(batch, flip);
    }

    direct3ddevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    if (!_screenTintSprite.skip)
    {
        this->_renderSprite(&_screenTintSprite, _spriteBatches[_actSpriteBatch].Matrix, false, false);
    }
}

void D3DGraphicsDriver::RenderSpriteBatch(const D3DSpriteBatch &batch, GlobalFlipType flip)
{
  bool globalLeftRightFlip = (flip == kFlip_Vertical) || (flip == kFlip_Both);
  bool globalTopBottomFlip = (flip == kFlip_Horizontal) || (flip == kFlip_Both);

  D3DDrawListEntry stageEntry; // raw-draw plugin support

  const std::vector<D3DDrawListEntry> &listToDraw = batch.List;
  for (size_t i = 0; i < listToDraw.size(); ++i)
  {
    if (listToDraw[i].skip)
      continue;

    const D3DDrawListEntry *sprite = &listToDraw[i];
    if (listToDraw[i].bitmap == NULL)
    {
      if (DoNullSpriteCallback(listToDraw[i].x, (int)direct3ddevice))
        stageEntry = D3DDrawListEntry((D3DBitmap*)_stageVirtualScreenDDB);
      else
        continue;
      sprite = &stageEntry;
    }

    this->_renderSprite(sprite, batch.Matrix, globalLeftRightFlip, globalTopBottomFlip);
  }
}

void D3DGraphicsDriver::InitSpriteBatch(size_t index, const SpriteBatchDesc &desc)
{
    if (_spriteBatches.size() <= index)
        _spriteBatches.resize(index + 1);
    _spriteBatches[index].List.clear();
    // Combine both world transform and viewport transform into one matrix for faster perfomance
    // TODO: find out if this is an optimal way to translate scaled room into Top-Left screen coordinates
    float scaled_offx = (_srcRect.GetWidth() - desc.Transform.ScaleX * (float)_srcRect.GetWidth()) / 2.f;
    float scaled_offy = (_srcRect.GetHeight() - desc.Transform.ScaleY * (float)_srcRect.GetHeight()) / 2.f;
    // TODO: correct offsets to have pre-scale (source) and post-scale (dest) offsets!
    // is it possible to do with matrixes?
    MatrixTransform2D(_spriteBatches[index].Matrix,
        desc.Transform.X + desc.Viewport.Left - scaled_offx, -(desc.Transform.Y + desc.Viewport.Top - scaled_offy),
        desc.Transform.ScaleX, desc.Transform.ScaleY, desc.Transform.Rotate);

    // create stage screen for plugin raw drawing
    int src_w = desc.Viewport.GetWidth() / desc.Transform.ScaleX;
    int src_h = desc.Viewport.GetHeight() / desc.Transform.ScaleY;
    CreateStageScreen(index, Size(src_w, src_h));
}

void D3DGraphicsDriver::ResetAllBatches()
{
    for (size_t i = 0; i < _spriteBatches.size(); ++i)
        _spriteBatches[i].List.clear();
}

void D3DGraphicsDriver::ClearDrawBackups()
{
    _backupBatchDescs.clear();
    _backupBatches.clear();
}

void D3DGraphicsDriver::BackupDrawLists()
{
    ClearDrawBackups();
    for (size_t i = 0; i <= _actSpriteBatch; ++i)
    {
        _backupBatchDescs.push_back(_spriteBatchDesc[i]);
        _backupBatches.push_back(_spriteBatches[i]);
    }
}

void D3DGraphicsDriver::RestoreDrawLists()
{
    if (_backupBatchDescs.size() == 0)
    {
        ClearDrawLists();
        return;
    }
    _spriteBatchDesc = _backupBatchDescs;
    _spriteBatches = _backupBatches;
    _actSpriteBatch = _backupBatchDescs.size() - 1;
}

void D3DGraphicsDriver::DrawSprite(int x, int y, IDriverDependantBitmap* bitmap)
{
    _spriteBatches[_actSpriteBatch].List.push_back(D3DDrawListEntry((D3DBitmap*)bitmap, x, y));
}

void D3DGraphicsDriver::DestroyDDB(IDriverDependantBitmap* bitmap)
{
    // Remove deleted DDB from backups
    for (D3DSpriteBatches::iterator it = _backupBatches.begin(); it != _backupBatches.end(); ++it)
    {
        std::vector<D3DDrawListEntry> &drawlist = it->List;
        for (size_t i = 0; i < drawlist.size(); i++)
        {
            if (drawlist[i].bitmap == bitmap)
                drawlist[i].skip = true;
        }
    }
    delete bitmap;
}

void D3DGraphicsDriver::UpdateTextureRegion(D3DTextureTile *tile, Bitmap *bitmap, D3DBitmap *target, bool hasAlpha)
{
  IDirect3DTexture9* newTexture = tile->texture;

  D3DLOCKED_RECT lockedRegion;
  HRESULT hr = newTexture->LockRect(0, &lockedRegion, NULL, D3DLOCK_NOSYSLOCK | D3DLOCK_DISCARD);
  if (hr != D3D_OK)
  {
    throw Ali3DException("Unable to lock texture");
  }

  bool usingLinearFiltering = _filter->NeedToColourEdgeLines();
  char *memPtr = (char*)lockedRegion.pBits;

  BitmapToVideoMem(bitmap, hasAlpha, tile, target, memPtr, lockedRegion.Pitch, usingLinearFiltering);

  newTexture->UnlockRect(0);
}

void D3DGraphicsDriver::UpdateDDBFromBitmap(IDriverDependantBitmap* bitmapToUpdate, Bitmap *bitmap, bool hasAlpha)
{
  D3DBitmap *target = (D3DBitmap*)bitmapToUpdate;
  if (target->_width != bitmap->GetWidth() || target->_height != bitmap->GetHeight())
    throw Ali3DException("UpdateDDBFromBitmap: mismatched bitmap size");
  const int color_depth = bitmap->GetColorDepth();
  if (color_depth != target->_colDepth)
    throw Ali3DException("UpdateDDBFromBitmap: mismatched colour depths");

  target->_hasAlpha = hasAlpha;
  if (color_depth == 8)
      select_palette(palette);

  for (int i = 0; i < target->_numTiles; i++)
  {
    UpdateTextureRegion(&target->_tiles[i], bitmap, target, hasAlpha);
  }

  if (color_depth == 8)
      unselect_palette();
}

int D3DGraphicsDriver::GetCompatibleBitmapFormat(int color_depth)
{
  if (color_depth == 8)
    return 8;
  if (color_depth > 8 && color_depth <= 16)
    return 16;
  return 32;
}

void D3DGraphicsDriver::AdjustSizeToNearestSupportedByCard(int *width, int *height)
{
  int allocatedWidth = *width, allocatedHeight = *height;

  if (direct3ddevicecaps.TextureCaps & D3DPTEXTURECAPS_POW2)
  {
    bool foundWidth = false, foundHeight = false;
    int tryThis = 2;
    while ((!foundWidth) || (!foundHeight))
    {
      if ((tryThis >= allocatedWidth) && (!foundWidth))
      {
        allocatedWidth = tryThis;
        foundWidth = true;
      }

      if ((tryThis >= allocatedHeight) && (!foundHeight))
      {
        allocatedHeight = tryThis;
        foundHeight = true;
      }

      tryThis = tryThis << 1;
    }
  }

  if (direct3ddevicecaps.TextureCaps & D3DPTEXTURECAPS_SQUAREONLY)
  {
    if (allocatedWidth > allocatedHeight) 
    {
     allocatedHeight = allocatedWidth;
    }
    else
    {
     allocatedWidth = allocatedHeight;
    }
  }

  *width = allocatedWidth;
  *height = allocatedHeight;
}

bool D3DGraphicsDriver::IsTextureFormatOk( D3DFORMAT TextureFormat, D3DFORMAT AdapterFormat ) 
{
    HRESULT hr = direct3d->CheckDeviceFormat( D3DADAPTER_DEFAULT,
                                          D3DDEVTYPE_HAL,
                                          AdapterFormat,
                                          0,
                                          D3DRTYPE_TEXTURE,
                                          TextureFormat);
    
    return SUCCEEDED( hr );
}

IDriverDependantBitmap* D3DGraphicsDriver::CreateDDBFromBitmap(Bitmap *bitmap, bool hasAlpha, bool opaque)
{
  int allocatedWidth = bitmap->GetWidth();
  int allocatedHeight = bitmap->GetHeight();
  if (bitmap->GetColorDepth() != GetCompatibleBitmapFormat(bitmap->GetColorDepth()))
    throw Ali3DException("CreateDDBFromBitmap: bitmap colour depth not supported");
  int colourDepth = bitmap->GetColorDepth();

  D3DBitmap *ddb = new D3DBitmap(bitmap->GetWidth(), bitmap->GetHeight(), colourDepth, opaque);

  AdjustSizeToNearestSupportedByCard(&allocatedWidth, &allocatedHeight);
  int tilesAcross = 1, tilesDown = 1;

  // *************** REMOVE THESE LINES *************
  //direct3ddevicecaps.MaxTextureWidth = 64;
  //direct3ddevicecaps.MaxTextureHeight = 256;
  // *************** END REMOVE THESE LINES *************

  // Calculate how many textures will be necessary to
  // store this image
  tilesAcross = (allocatedWidth + direct3ddevicecaps.MaxTextureWidth - 1) / direct3ddevicecaps.MaxTextureWidth;
  tilesDown = (allocatedHeight + direct3ddevicecaps.MaxTextureHeight - 1) / direct3ddevicecaps.MaxTextureHeight;
  int tileWidth = bitmap->GetWidth() / tilesAcross;
  int lastTileExtraWidth = bitmap->GetWidth() % tilesAcross;
  int tileHeight = bitmap->GetHeight() / tilesDown;
  int lastTileExtraHeight = bitmap->GetHeight() % tilesDown;
  int tileAllocatedWidth = tileWidth;
  int tileAllocatedHeight = tileHeight;

  AdjustSizeToNearestSupportedByCard(&tileAllocatedWidth, &tileAllocatedHeight);

  int numTiles = tilesAcross * tilesDown;
  D3DTextureTile *tiles = (D3DTextureTile*)malloc(sizeof(D3DTextureTile) * numTiles);
  memset(tiles, 0, sizeof(D3DTextureTile) * numTiles);

  CUSTOMVERTEX *vertices = NULL;

  if ((numTiles == 1) &&
      (allocatedWidth == bitmap->GetWidth()) &&
      (allocatedHeight == bitmap->GetHeight()))
  {
    // use default whole-image vertices
  }
  else
  {
     // The texture is not the same as the bitmap, so create some custom vertices
     // so that only the relevant portion of the texture is rendered
     int vertexBufferSize = numTiles * 4 * sizeof(CUSTOMVERTEX);
     HRESULT hr = direct3ddevice->CreateVertexBuffer(vertexBufferSize, 0,
         D3DFVF_CUSTOMVERTEX, D3DPOOL_MANAGED, &ddb->_vertex, NULL);

     if (hr != D3D_OK) 
     {
        char errorMessage[200];
        sprintf(errorMessage, "Direct3DDevice9::CreateVertexBuffer(Length=%d) for texture failed: error code %08X", vertexBufferSize, hr);
        throw Ali3DException(errorMessage);
     }

     if (ddb->_vertex->Lock(0, 0, (void**)&vertices, D3DLOCK_DISCARD) != D3D_OK)
     {
       throw Ali3DException("Failed to lock vertex buffer");
     }
  }

  for (int x = 0; x < tilesAcross; x++)
  {
    for (int y = 0; y < tilesDown; y++)
    {
      D3DTextureTile *thisTile = &tiles[y * tilesAcross + x];
      int thisAllocatedWidth = tileAllocatedWidth;
      int thisAllocatedHeight = tileAllocatedHeight;
      thisTile->x = x * tileWidth;
      thisTile->y = y * tileHeight;
      thisTile->width = tileWidth;
      thisTile->height = tileHeight;
      if (x == tilesAcross - 1) 
      {
        thisTile->width += lastTileExtraWidth;
        thisAllocatedWidth = thisTile->width;
        AdjustSizeToNearestSupportedByCard(&thisAllocatedWidth, &thisAllocatedHeight);
      }
      if (y == tilesDown - 1) 
      {
        thisTile->height += lastTileExtraHeight;
        thisAllocatedHeight = thisTile->height;
        AdjustSizeToNearestSupportedByCard(&thisAllocatedWidth, &thisAllocatedHeight);
      }

      if (vertices != NULL)
      {
        for (int vidx = 0; vidx < 4; vidx++)
        {
          int i = (y * tilesAcross + x) * 4 + vidx;
          vertices[i] = defaultVertices[vidx];
          if (vertices[i].tu > 0.0)
          {
            vertices[i].tu = (float)thisTile->width / (float)thisAllocatedWidth;
          }
          if (vertices[i].tv > 0.0)
          {
            vertices[i].tv = (float)thisTile->height / (float)thisAllocatedHeight;
          }
        }
      }

      // NOTE: pay attention that the texture format depends on the **display mode**'s color format,
      // rather than source bitmap's color depth!
      D3DFORMAT textureFormat = color_depth_to_d3d_format(_mode.ColorDepth, !opaque);
      HRESULT hr = direct3ddevice->CreateTexture(thisAllocatedWidth, thisAllocatedHeight, 1, 0, 
                                      textureFormat,
                                      D3DPOOL_MANAGED, &thisTile->texture, NULL);
      if (hr != D3D_OK)
      {
        char errorMessage[200];
        sprintf(errorMessage, "Direct3DDevice9::CreateTexture(X=%d, Y=%d, FMT=%d) failed: error code %08X", thisAllocatedWidth, thisAllocatedHeight, textureFormat, hr);
        throw Ali3DException(errorMessage);
      }

    }
  }

  if (vertices != NULL)
  {
    ddb->_vertex->Unlock();
  }

  ddb->_numTiles = numTiles;
  ddb->_tiles = tiles;

  UpdateDDBFromBitmap(ddb, bitmap, hasAlpha);

  return ddb;
}

void D3DGraphicsDriver::do_fade(bool fadingOut, int speed, int targetColourRed, int targetColourGreen, int targetColourBlue)
{
  if (fadingOut)
  {
    this->_reDrawLastFrame();
  }
  else if (_drawScreenCallback != NULL)
    _drawScreenCallback();
  
  Bitmap *blackSquare = BitmapHelper::CreateBitmap(16, 16, 32);
  blackSquare->Clear(makecol32(targetColourRed, targetColourGreen, targetColourBlue));
  IDriverDependantBitmap *d3db = this->CreateDDBFromBitmap(blackSquare, false, false);
  delete blackSquare;

  BeginSpriteBatch(_srcRect, SpriteTransform());
  d3db->SetStretch(_srcRect.GetWidth(), _srcRect.GetHeight(), false);
  // NOTE: what happens here is that we are trying to prevent global offset to be applied to this sprite :/
  this->DrawSprite(-_globalViewOff.X, -_globalViewOff.Y, d3db);

  if (speed <= 0) speed = 16;
  speed *= 2;  // harmonise speeds with software driver which is faster
  for (int a = 1; a < 255; a += speed)
  {
    int timerValue = *_loopTimer;
    d3db->SetTransparency(fadingOut ? a : (255 - a));
    this->_renderAndPresent(flipTypeLastTime, false);

    do
    {
      if (_pollingCallback)
        _pollingCallback();
      platform->YieldCPU();
    }
    while (timerValue == *_loopTimer);

  }

  if (fadingOut)
  {
    d3db->SetTransparency(0);
    this->_renderAndPresent(flipTypeLastTime, false);
  }

  this->DestroyDDB(d3db);
  this->ClearDrawLists();
}

void D3DGraphicsDriver::FadeOut(int speed, int targetColourRed, int targetColourGreen, int targetColourBlue) 
{
  do_fade(true, speed, targetColourRed, targetColourGreen, targetColourBlue);
}

void D3DGraphicsDriver::FadeIn(int speed, PALLETE p, int targetColourRed, int targetColourGreen, int targetColourBlue) 
{
  do_fade(false, speed, targetColourRed, targetColourGreen, targetColourBlue);
}

void D3DGraphicsDriver::BoxOutEffect(bool blackingOut, int speed, int delay)
{
  if (blackingOut)
  {
    this->_reDrawLastFrame();
  }
  else if (_drawScreenCallback != NULL)
    _drawScreenCallback();
  
  Bitmap *blackSquare = BitmapHelper::CreateBitmap(16, 16, 32);
  blackSquare->Clear();
  IDriverDependantBitmap *d3db = this->CreateDDBFromBitmap(blackSquare, false, false);
  delete blackSquare;

  BeginSpriteBatch(_srcRect, SpriteTransform());
  d3db->SetStretch(_srcRect.GetWidth(), _srcRect.GetHeight(), false);
  // NOTE: what happens here is that we are trying to prevent global offset to be applied to this sprite :/
  this->DrawSprite(-_globalViewOff.X, -_globalViewOff.Y, d3db);
  if (!blackingOut)
  {
    // when fading in, draw four black boxes, one
    // across each side of the screen
    this->DrawSprite(0, 0, d3db);
    this->DrawSprite(0, 0, d3db);
    this->DrawSprite(0, 0, d3db);
  }

  int yspeed = _srcRect.GetHeight() / (_srcRect.GetWidth() / speed);
  int boxWidth = speed;
  int boxHeight = yspeed;

  while (boxWidth < _srcRect.GetWidth())
  {
    boxWidth += speed;
    boxHeight += yspeed;
    D3DSpriteBatch &batch = _spriteBatches.back();
    std::vector<D3DDrawListEntry> &drawList = batch.List;
    const size_t last = drawList.size() - 1;
    if (blackingOut)
    {
      drawList[last].x = _srcRect.GetWidth() / 2- boxWidth / 2;
      drawList[last].y = _srcRect.GetHeight() / 2 - boxHeight / 2;
      d3db->SetStretch(boxWidth, boxHeight, false);
    }
    else
    {
      drawList[last - 3].x = _srcRect.GetWidth() / 2 - boxWidth / 2 - _srcRect.GetWidth();
      drawList[last - 2].y = _srcRect.GetHeight() / 2 - boxHeight / 2 - _srcRect.GetHeight();
      drawList[last - 1].x = _srcRect.GetWidth() / 2 + boxWidth / 2;
      drawList[last    ].y = _srcRect.GetHeight() / 2 + boxHeight / 2;
      d3db->SetStretch(_srcRect.GetWidth(), _srcRect.GetHeight(), false);
    }
    
    this->_renderAndPresent(flipTypeLastTime, false);

    if (_pollingCallback)
      _pollingCallback();
    platform->Delay(delay);
  }

  this->DestroyDDB(d3db);
  this->ClearDrawLists();
}

bool D3DGraphicsDriver::PlayVideo(const char *filename, bool useAVISound, VideoSkipType skipType, bool stretchToFullScreen)
{
  direct3ddevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_RGBA(0, 0, 0, 255), 0.5f, 0);

  int result = dxmedia_play_video_3d(filename, direct3ddevice, useAVISound, skipType, stretchToFullScreen ? 1 : 0);
  return (result == 0);
}

void D3DGraphicsDriver::create_screen_tint_bitmap() 
{
  // Some work on textures depends on current scaling filter, sadly
  // TODO: find out if there is a workaround for that
  if (!IsModeSet() || !_filter)
    return;

  _screenTintLayer = BitmapHelper::CreateBitmap(16, 16, this->_mode.ColorDepth);
  _screenTintLayerDDB = (D3DBitmap*)this->CreateDDBFromBitmap(_screenTintLayer, false, false);
  _screenTintSprite.bitmap = _screenTintLayerDDB;
}

void D3DGraphicsDriver::SetScreenTint(int red, int green, int blue)
{ 
  if ((red != _tint_red) || (green != _tint_green) || (blue != _tint_blue))
  {
    _tint_red = red; 
    _tint_green = green; 
    _tint_blue = blue;

    _screenTintLayer->Clear(makecol_depth(_screenTintLayer->GetColorDepth(), red, green, blue));
    this->UpdateDDBFromBitmap(_screenTintLayerDDB, _screenTintLayer, false);
    _screenTintLayerDDB->SetStretch(_srcRect.GetWidth(), _srcRect.GetHeight(), false);
    _screenTintLayerDDB->SetTransparency(128);

    _screenTintSprite.skip = ((red == 0) && (green == 0) && (blue == 0));
  }
}


D3DGraphicsFactory *D3DGraphicsFactory::_factory = NULL;
Library D3DGraphicsFactory::_library;

D3DGraphicsFactory::~D3DGraphicsFactory()
{
    DestroyDriver(); // driver must be destroyed before d3d library is disposed
    ULONG ref_cnt = _direct3d->Release();
    if (ref_cnt > 0)
        Debug::Printf(kDbgMsg_Warn, "WARNING: Not all of the Direct3D resources have been disposed; ID3D ref count: %d", ref_cnt);
    _factory = NULL;
}

size_t D3DGraphicsFactory::GetFilterCount() const
{
    return 2;
}

const GfxFilterInfo *D3DGraphicsFactory::GetFilterInfo(size_t index) const
{
    switch (index)
    {
    case 0:
        return &D3DGfxFilter::FilterInfo;
    case 1:
        return &AAD3DGfxFilter::FilterInfo;
    default:
        return NULL;
    }
}

String D3DGraphicsFactory::GetDefaultFilterID() const
{
    return D3DGfxFilter::FilterInfo.Id;
}

/* static */ D3DGraphicsFactory *D3DGraphicsFactory::GetFactory()
{
    if (!_factory)
    {
        _factory = new D3DGraphicsFactory();
        if (!_factory->Init())
        {
            delete _factory;
            _factory = NULL;
        }
    }
    return _factory;
}

/* static */ D3DGraphicsDriver *D3DGraphicsFactory::GetD3DDriver()
{
    if (!_factory)
        _factory = GetFactory();
    if (_factory)
        return _factory->EnsureDriverCreated();
    return NULL;
}


D3DGraphicsFactory::D3DGraphicsFactory()
    : _direct3d(NULL)
{
}

D3DGraphicsDriver *D3DGraphicsFactory::EnsureDriverCreated()
{
    if (!_driver)
    {
        _factory->_direct3d->AddRef();
        _driver = new D3DGraphicsDriver(_factory->_direct3d);
    }
    return _driver;
}

D3DGfxFilter *D3DGraphicsFactory::CreateFilter(const String &id)
{
    if (D3DGfxFilter::FilterInfo.Id.CompareNoCase(id) == 0)
        return new D3DGfxFilter();
    else if (AAD3DGfxFilter::FilterInfo.Id.CompareNoCase(id) == 0)
        return new AAD3DGfxFilter();
    return NULL;
}

bool D3DGraphicsFactory::Init()
{
    assert(_direct3d == NULL);
    if (_direct3d)
        return true;

    if (!_library.Load("d3d9"))
    {
        set_allegro_error("Direct3D 9 is not installed");
        return false;
    }

    typedef IDirect3D9 * WINAPI D3D9CreateFn(UINT);
    D3D9CreateFn *lpDirect3DCreate9 = (D3D9CreateFn*)_library.GetFunctionAddress("Direct3DCreate9");
    if (!lpDirect3DCreate9)
    {
        _library.Unload();
        set_allegro_error("Entry point not found in d3d9.dll");
        return false;
    }
    _direct3d = lpDirect3DCreate9(D3D_SDK_VERSION);
    if (!_direct3d)
    {
        _library.Unload();
        set_allegro_error("Direct3DCreate failed!");
        return false;
    }
    return true;
}

} // namespace D3D
} // namespace Engine
} // namespace AGS
