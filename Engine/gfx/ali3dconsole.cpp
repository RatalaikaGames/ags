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
// Allegro Interface for 3D; Game consoles driver
//
//=============================================================================

#ifndef CONSOLE_VERSION
#error This file should only be included on the Console build
#endif

#include <allegro.h>
#include <cmath>

#include "main/main_allegro.h"
#include "debug/assert.h"
#include "debug/out.h"
#include "platform/base/agsplatformdriver.h"
#include "gfx/ali3dexception.h"
#include "gfx/gfxfilter_console.h"
#include "gfx/gfxfilter_aaconsole.h"
#include "gfx/gfx_util.h"
#include "gfx/ali3dconsole.h"
#include "util/library.h"
#include "media/audio/audio.h"

using namespace AGS::Common;

// Necessary to update textures from 8-bit bitmaps
extern RGB palette[256];

namespace AGS
{
	namespace Engine
	{
		namespace AL3DConsole
		{
			//singleton instance
			static ConsoleGraphicsDriver s_inst(0,0);

			using namespace Common;

			// Setup identity matrix
			void MatrixIdentity(Matrix44 &m)
			{
				m = {
					1.0, 0.0, 0.0, 0.0,
					0.0, 1.0, 0.0, 0.0,
					0.0, 0.0, 1.0, 0.0,
					0.0, 0.0, 0.0, 1.0
				};
			}
			// Setup translation matrix
			void MatrixTranslate(Matrix44 &m, float x, float y, float z)
			{
				MatrixIdentity(m);
				m.m[3][0] = x;
				m.m[3][1] = y;
				m.m[3][2] = z;
			}
			// Setup scaling matrix
			void MatrixScale(Matrix44 &m, float sx, float sy, float sz)
			{
				MatrixIdentity(m);
				m.m[0][0] = sx;
				m.m[1][1] = sy;
				m.m[2][2] = sz;
			}
			// Setup rotation around Z axis; angle is in radians
			void MatrixRotateZ(Matrix44 &m, float angle)
			{
				MatrixIdentity(m);
				m.m[0][0] = std::cos(angle);
				m.m[1][1] = std::cos(angle);
				m.m[0][1] = std::sin(angle);
				m.m[1][0] = -std::sin(angle);
			}
			// Matrix multiplication
			void MatrixMultiply(Matrix44 &mr, const Matrix44 &m1, const Matrix44 &m2)
			{
				for (int i = 0; i < 4; ++i)
					for (int j = 0; j < 4; ++j)
						mr.m[i][j] = m1.m[i][0] * m2.m[0][j] + m1.m[i][1] * m2.m[1][j] + m1.m[i][2] * m2.m[2][j] + m1.m[i][3] * m2.m[3][j];
			}
			// Setup full 2D transformation matrix
			void MatrixTransform2D(Matrix44 &m, float x, float y, float sx, float sy, float anglez)
			{
				Matrix44 translate;
				Matrix44 scale;
				Matrix44 rotate;
				MatrixTranslate(translate, x, y, 0.f);
				MatrixScale(scale, sx, sy, 1.f);
				MatrixRotateZ(rotate, anglez);

				Matrix44 tr1;
				MatrixMultiply(tr1, rotate, scale);
				MatrixMultiply(m, tr1, translate);
			}

			void DDBitmap::Dispose()
			{
				if (_tiles != nullptr)
				{
					for (int i = 0; i < _numTiles; i++)
						AGSCON::Graphics::Texture_Destroy(_tiles[i].texture);

					free(_tiles);
					_tiles = nullptr;
					_numTiles = 0;
				}
				if (_vertex != nullptr)
				{
					
					AGSCON::Graphics::VertexBuffer_Destroy(_vertex);
					_vertex = nullptr;
				}
			}

			void dummy_vsync() { }

			#define GFX_CONSOLE AL_ID('C','O','N','S')

			GFX_DRIVER gfx_direct3d_win =
			{
				GFX_CONSOLE,
				empty_string,
				empty_string,
				"Console",
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

			ConsoleGraphicsDriver::ConsoleGraphicsDriver(int secret, float key) {}

			void ConsoleGraphicsDriver::Init()
			{
				if(_initialized) return;
				_initialized = true;

				AGSCON::Graphics::Initialize();

				_legacyPixelShader = false;
				//pNativeSurface = NULL;
				_smoothScaling = false;
				_pixelRenderXOffset = 0;
				_pixelRenderYOffset = 0;
				_renderSprAtScreenRes = false;

				const int* shifts = AGSCON::Graphics::GetColorShifts();
				_vmem_a_shift_32 = shifts[0];
				_vmem_r_shift_32 = shifts[1];
				_vmem_g_shift_32 = shifts[2];
				_vmem_b_shift_32 = shifts[3];

				// Initialize default sprite batch, it will be used when no other batch was activated
				InitSpriteBatch(0, _spriteBatchDesc[0]);

				//set up "default vertices" used as a template for making other verts
				defaultVertices[0].position.x = 0.0f;
				defaultVertices[0].position.y = 0.0f;
				defaultVertices[0].tu=0.0;
				defaultVertices[0].tv=0.0;
				defaultVertices[1].position.x = 1.0f;
				defaultVertices[1].position.y = 0.0f;
				defaultVertices[1].tu=1.0;
				defaultVertices[1].tv=0.0;
				defaultVertices[2].position.x = 0.0f;
				defaultVertices[2].position.y = -1.0f;
				defaultVertices[2].tu=0.0;
				defaultVertices[2].tv=1.0;
				defaultVertices[3].position.x = 1.0f;
				defaultVertices[3].position.y = -1.0f;
				defaultVertices[3].tu=1.0;
				defaultVertices[3].tv=1.0;
			}
			
			void ConsoleGraphicsDriver::Vsync() 
			{
				// do nothing on D3D
			}


			//useful reminder of how to shut down, I guess
			//void ConsoleGraphicsDriver::ReleaseDisplayMode()
			//{
			//	ClearDrawLists();
			//	ClearDrawBackups();

			//	if (_screenTintLayerDDB != NULL) 
			//	{
			//		this->DestroyDDB(_screenTintLayerDDB);
			//		_screenTintLayerDDB = NULL;
			//		_screenTintSprite.bitmap = NULL;
			//	}
			//	delete _screenTintLayer;
			//	_screenTintLayer = NULL;

			//	DestroyAllStageScreens();
			//}

			bool ConsoleGraphicsDriver::IsModeSupported(const DisplayMode &mode)
			{
				if (mode.Width <= 0 || mode.Height <= 0 || mode.ColorDepth <= 0)
				{
					set_allegro_error("Invalid resolution parameters: %d x %d x %d", mode.Width, mode.Height, mode.ColorDepth);
					return false;
				}

				set_allegro_error("The requested adapter mode is not supported");
				return false;
			}

			bool ConsoleGraphicsDriver::SupportsGammaControl() 
			{
				//support when I find a game that needs it
				return true;
			}

			void ConsoleGraphicsDriver::SetGamma(int newGamma)
			{
				//I'm saving this as a reference of how people expect it to work, but I'll implement it differently if I do support it
				/*for (int i = 0; i < 256; i++) 
				{
					int newValue = ((int)defaultgammaramp.red[i] * newGamma) / 100;
					if (newValue >= 65535)
						newValue = 65535;
					currentgammaramp.red,green,blue[i] = newValue;
				}*/

				//This will be implementation-defined.. since it kind of was to begin with (insofar as it wasn't likely to actually work for anyone, nor be an actual gamma adjustment)
				AGSCON::Graphics::SetGamma(newGamma);
			}

			void ConsoleGraphicsDriver::InitializeDDState()
			{
				//initialize cull, blend, Ztest, etc.
			}

			void ConsoleGraphicsDriver::SetupViewport()
			{
				// Setup orthographic projection matrix
				Matrix44 matOrtho = {
					(2.0f / _srcRect.GetWidth()), 0.0f, 0.0f, 0.0f,
					0.0f, (2.0f / _srcRect.GetHeight()), 0.0f, 0.0f,
					0.0f, 0.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 0.0f, 1.0f
				};
				currentProjection = matOrtho;

				Matrix44 matIdentity = {
					1.0f, 0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f, 1.0f
				};

				//reminder if needed for testing on PC or d3d11
				//_pixelRenderXOffset = ((float)_srcRect.GetWidth() / (float)_mode.Width) / 2.0f;
				//_pixelRenderYOffset = ((float)_srcRect.GetHeight() / (float)_mode.Height) / 2.0f;

				//REMINDER of possibly expected semantics: does anyone actually use this?
				// Clear the screen before setting a viewport.
				//ClearRectangle(0, 0, _mode.Width, _mode.Height, 0);

				//note the odd semantics here. 
				//what do we use this for anyway? we have _srcRect and _dstRect cached
				viewport_rect.left   = _dstRect.Left;
				viewport_rect.right  = _dstRect.Right + 1;
				viewport_rect.top    = _dstRect.Top;
				viewport_rect.bottom = _dstRect.Bottom + 1;
			}

			void ConsoleGraphicsDriver::SetGraphicsFilter(PConsoleFilter filter)
			{
				_filter = filter;
				OnSetFilter();
			}

			void ConsoleGraphicsDriver::SetTintMethod(TintMethod method) 
			{
				_legacyPixelShader = (method == TintReColourise);
			}

			bool ConsoleGraphicsDriver::SetDisplayMode(const DisplayMode &mode, volatile int *loopTimer)
			{
				//we'll hack games if this is inadequate
				if (mode.ColorDepth != 32)
				{
					set_allegro_error("THIS driver does not support THAT display mode");
					return false;
				}

				OnInit(loopTimer);
				GraphicsDriverBase::OnModeSet(mode);
				InitializeDDState();
				CreateVirtualScreen();
				return true;
			}

			void ConsoleGraphicsDriver::CreateVirtualScreen()
			{
				if (!IsModeSet() || !IsNativeSizeValid())
					return;

				// set up native surface
				if (pNativeSurface != NULL)
				{
					AGSCON::Graphics::RenderTarget_Destroy(pNativeSurface);
					pNativeSurface = NULL;
				}
				pNativeSurface = AGSCON::Graphics::RenderTarget_Create(_srcRect.GetWidth(), _srcRect.GetHeight());
				
				//TODO: clear to 0 (required semantics, probably)
				//however, clearing may be trouble because we aren't drawing now.

				// create initial stage screen for plugin raw drawing
				_stageVirtualScreen = CreateStageScreen(0, _srcRect.GetSize());
				// we must set Allegro's screen pointer to **something**
				screen = (BITMAP*)_stageVirtualScreen->GetAllegroBitmap();
			}

			bool ConsoleGraphicsDriver::SetNativeSize(const Size &src_size)
			{
				OnSetNativeSize(src_size);
				// Also make sure viewport is updated using new native & destination rectangles
				SetupViewport();
				CreateVirtualScreen();
				return !_srcRect.IsEmpty();
			}

			bool ConsoleGraphicsDriver::SetRenderFrame(const Rect &dst_rect)
			{
				OnSetRenderFrame(dst_rect);
				// Also make sure viewport is updated using new native & destination rectangles
				SetupViewport();
				return !_dstRect.IsEmpty();
			}

			int ConsoleGraphicsDriver::GetDisplayDepthForNativeDepth(int native_color_depth) const
			{
				// TODO: check for device caps to know which depth is supported?
				return 32;
			}

			static class DDGfxModeList : public IGfxModeList
			{
			public:
				virtual int GetModeCount() const { return 1; }
				virtual bool GetMode(int index, DisplayMode &mode) const {
					
					//just guessing for now. Don't know if I really need this.
					static const GraphicResolution rez(1280,720,32);
					static const DisplayMode modes[] = {
						DisplayMode(rez, true, 60, true)
					};
					if(index < 0) return false;
					if(index > 1) return false;
					mode = modes[0];
					return true;
				}
			} s_modeList;

			IGfxModeList *ConsoleGraphicsDriver::GetSupportedModeList(int color_depth)
			{
				return &s_modeList;
			}

			PGfxFilter ConsoleGraphicsDriver::GetGraphicsFilter() const
			{
				return _filter;
			}

			void ConsoleGraphicsDriver::UnInit() 
			{
				if(!_initialized) return;

				//who wants to deal with this?
				OnUnInit();

				_initialized = false;
			}

			ConsoleGraphicsDriver::~ConsoleGraphicsDriver()
			{
				UnInit();
			}

			void ConsoleGraphicsDriver::ClearRectangle(int x1, int y1, int x2, int y2, RGB *colorToUse)
			{
				//I'm leaving this here as a note, but.... why the NOTE below?
				//a game may clear a rectangle for any reason
				//// NOTE: this function is practically useless at the moment, because D3D redraws whole game frame each time
				//Rect r(x1, y1, x2, y2);
				//r = _scaling.ScaleRange(r);
				//D3DRECT rectToClear;
				//rectToClear.x1 = r.Left;
				//rectToClear.y1 = r.Top;
				//rectToClear.x2 = r.Right + 1;
				//rectToClear.y2 = r.Bottom + 1;
				//DWORD colorDword = 0;
				//if (colorToUse != NULL)
				//	colorDword = D3DCOLOR_XRGB(colorToUse->r, colorToUse->g, colorToUse->b);
				//direct3ddevice->Clear(1, &rectToClear, D3DCLEAR_TARGET, colorDword, 0.5f, 0);
			}

			bool ConsoleGraphicsDriver::GetCopyOfScreenIntoBitmap(Bitmap *destination, bool at_native_res, GraphicResolution *want_fmt)
			{
				//-------------------------------------------
				//MBG AUG 2019 - disabled due to merge, dont think we need it
				//// Currently don't support copying in screen resolution when we are rendering in native
				//if (!_renderSprAtScreenRes)
				//	at_native_res = true;
				//Size need_size = at_native_res ? _srcRect.GetSize() : _dstRect.GetSize();
				//if (destination->GetColorDepth() != _mode.ColorDepth || destination->GetSize() != need_size)
				//{
				//	if (want_size)
				//		*want_size = need_size;
				//	return false;
				//}
				//// If we are rendering sprites at the screen resolution, and requested native res,
				//// re-render last frame to the native surface
				//if (at_native_res && _renderSprAtScreenRes)
				//{
				//	_renderSprAtScreenRes = false;
				//	_reDrawLastFrame();
				//	_render(true);
				//	_renderSprAtScreenRes = true;
				//}
				//-------------------------------------------

				//THIS IS NOT GOING TO BE PRETTY!!

				//IDirect3DSurface9* surface = NULL;
				//{
				//	if (_pollingCallback)
				//		_pollingCallback();

				//	if (at_native_res)
				//	{
				//		surface = pNativeSurface;
				//	}
				//	// Get the back buffer surface
				//	else if (direct3ddevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surface) != D3D_OK)
				//	{
				//		throw Ali3DException("IDirect3DDevice9::GetBackBuffer failed");
				//	}

				//	if (_pollingCallback)
				//		_pollingCallback();

				//	D3DLOCKED_RECT lockedRect;
				//	if (surface->LockRect(&lockedRect, (at_native_res ? NULL : &viewport_rect), D3DLOCK_READONLY ) != D3D_OK)
				//	{
				//		throw Ali3DException("IDirect3DSurface9::LockRect failed");
				//	}

				//	BitmapHelper::ReadPixelsFromMemory(destination, (uint8_t*)lockedRect.pBits, lockedRect.Pitch);

				//	surface->UnlockRect();
				//	if (!at_native_res) // native surface is released elsewhere
				//		surface->Release();

				//	if (_pollingCallback)
				//		_pollingCallback();
				//}

				return true;
			}

			void ConsoleGraphicsDriver::RenderToBackBuffer()
			{
				throw Ali3DException("driver does not have a back buffer");
			}

			void ConsoleGraphicsDriver::Render()
			{
				Render(kFlip_None);
			}

			void ConsoleGraphicsDriver::Render()
			{
				_renderAndPresent(true);
			}

			void ConsoleGraphicsDriver::_reDrawLastFrame()
			{
				RestoreDrawLists();
			}

			void ConsoleGraphicsDriver::_renderSprite(const DDDrawListEntry *drawListEntry, const Matrix44 &matGlobal)
			{
				DDBitmap *bmpToDraw = drawListEntry->bitmap;
				Matrix44 matSelfTransform;
				Matrix44 matTransform;

				if (bmpToDraw->_transparency >= 255)
					return;

				AGSCON::Graphics::SHADERS::StandardProgram* selectedProgram = nullptr;

				if (bmpToDraw->_tintSaturation > 0)
				{
					// RATA: HACK!! For some reason this does the trick, using the HSV shader doesn't seems to work?!
					_legacyPixelShader = false;

					if (_legacyPixelShader)
						selectedProgram = &AGSCON::Graphics::shaders.tintLegacy;
					else
						selectedProgram = &AGSCON::Graphics::shaders.tint;
					AGSCON::Graphics::BindProgram(selectedProgram->program);

					// Use custom pixel shader
					float vector[] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

					if (_legacyPixelShader)
					{
						rgb_to_hsv(bmpToDraw->_red, bmpToDraw->_green, bmpToDraw->_blue, &vector[0], &vector[1], &vector[2]);
						vector[0] /= 360.0; // In HSV, Hue is 0-360
					}
					else
					{
						vector[0] = (float)bmpToDraw->_red / 255.0f;
						vector[1] = (float)bmpToDraw->_green / 255.0f;
						vector[2] = (float)bmpToDraw->_blue / 255.0f;
					}

					vector[3] = (float)bmpToDraw->_tintSaturation / 255.0f;

					if (bmpToDraw->_transparency > 0)
						vector[4] = (float)bmpToDraw->_transparency / 255.0f;
					else
						vector[4] = 1.0f;

					if (bmpToDraw->_lightLevel > 0)
						vector[5] = (float)bmpToDraw->_lightLevel / 255.0f;
					else
						vector[5] = 1.0f;

					if (_legacyPixelShader)
					{
						AGSCON::Graphics::UniformFloat4(AGSCON::Graphics::shaders.tintLegacy.tintRGB, &vector[0]);
						AGSCON::Graphics::UniformFloat4(AGSCON::Graphics::shaders.tintLegacy.transparency, &vector[4]);
					}
					else
					{
						AGSCON::Graphics::UniformFloat4(AGSCON::Graphics::shaders.tint.tintRGB, &vector[0]);
						AGSCON::Graphics::UniformFloat4(AGSCON::Graphics::shaders.tint.transparency, &vector[4]);
					}
				}
				else
				{
					selectedProgram = &AGSCON::Graphics::shaders.standard;
					AGSCON::Graphics::BindProgram(selectedProgram->program);

					int useTintRed = 255;
					int useTintGreen = 255;
					int useTintBlue = 255;
					int useTransparency = 0xff;
					
					//control.x == 0 -> TEX+TFACTOR
					//control.x == 1 -> TEX*TFACTOR
					//control.y reserved for other things
					float control[] = { 1.0f, 0.0f };

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
						// "ideally we would use a multi-stage operation here
						// because we need to do TEXTURE + (TEXTURE x LIGHT)"
						//OK, so why not? but we should leave it this way for compatibility's sake
						std::swap(control[0],control[1]);
						useTintRed = (bmpToDraw->_lightLevel - 256) / 2;
						useTintGreen = useTintRed;
						useTintBlue = useTintRed;
					}

					if (bmpToDraw->_transparency > 0)
					{
						useTransparency = bmpToDraw->_transparency;
					}

					float textureFactor[] = {
						useTintRed/255.0f,
						useTintGreen/255.0f,
						useTintBlue/255.0f,
						useTransparency/255.0f
					};

					// "No transparency, use texture alpha component"
					// In other words:
					// Just select the texture alpha by clobbering modulation alpha channel to 1.0;
					// else modulate by the selected transparency
					if(bmpToDraw->_transparency == 0)
						textureFactor[3] = 1.0f;

					AGSCON::Graphics::UniformFloat4(AGSCON::Graphics::shaders.standard.uTextureFactor,textureFactor);
					AGSCON::Graphics::UniformFloat2(AGSCON::Graphics::shaders.standard.uControl,control);
				}

				AGSCON::Graphics::BindVertexBuffer(bmpToDraw->_vertex);

				float width = bmpToDraw->GetWidthToRender();
				float height = bmpToDraw->GetHeightToRender();
				float xProportion = width / (float)bmpToDraw->_width;
				float yProportion = height / (float)bmpToDraw->_height;

				float drawAtX = drawListEntry->x + _globalViewOff.X;
				float drawAtY = drawListEntry->y + _globalViewOff.Y;

				for (int ti = 0; ti < bmpToDraw->_numTiles; ti++)
				{
					width = bmpToDraw->_tiles[ti].width * xProportion;
					height = bmpToDraw->_tiles[ti].height * yProportion;
					float xOffs;
					float yOffs = bmpToDraw->_tiles[ti].y * yProportion;
					if (bmpToDraw->_flipped)
						xOffs = (bmpToDraw->_width - (bmpToDraw->_tiles[ti].x + bmpToDraw->_tiles[ti].width)) * xProportion;
					else
						xOffs = bmpToDraw->_tiles[ti].x * xProportion;
					float thisX = drawAtX + xOffs;
					float thisY = drawAtY + yOffs;

					thisX = (-(_srcRect.GetWidth() / 2)) + thisX;
					thisY = (_srcRect.GetHeight() / 2) - thisY;

					//Setup translation and scaling matrices
					float widthToScale = (float)width;
					float heightToScale = (float)height;
					if (bmpToDraw->_flipped)
					{
						// The usual transform changes 0..1 into 0..width
						// So first negate it (which changes 0..w into -w..0)
						widthToScale = -widthToScale;
						// and now shift it over to make it 0..w again
						thisX += width;
					}

					// Multiply object's own and global matrixes
					MatrixTransform2D(matSelfTransform, (float)thisX - _pixelRenderXOffset, (float)thisY + _pixelRenderYOffset, widthToScale, heightToScale, 0.f);
					MatrixMultiply(matTransform, matSelfTransform, matGlobal);

					AGSCON::Graphics::UniformMatrix44(selectedProgram->um44Projection, (float*)&currentProjection);
					AGSCON::Graphics::UniformMatrix44(selectedProgram->um44Modelview, (float*)&matTransform);

					AGSCON::Graphics::Sampler* sampler;
					if ((_smoothScaling) && bmpToDraw->_useResampler && (bmpToDraw->_stretchToHeight > 0) &&
						((bmpToDraw->_stretchToHeight != bmpToDraw->_height) ||
						(bmpToDraw->_stretchToWidth != bmpToDraw->_width)))
					{
						sampler = AGSCON::Graphics::samplers.linear;
					}
					else if (!_renderSprAtScreenRes)
					{
						sampler = AGSCON::Graphics::samplers.nearest;
					}
					else
					{
						sampler = AGSCON::Graphics::samplers.nearest;
					}
					
					AGSCON::Graphics::BindFragmentTexture(selectedProgram->tex, bmpToDraw->_tiles[ti].texture, sampler, selectedProgram->uTextureControl);

					AGSCON::Graphics::DrawVertices(ti*4,4, AGSCON::Graphics::PrimitiveType::TriangleStrip);
				}
			}

			void ConsoleGraphicsDriver::_renderAndPresent(bool clearDrawListAfterwards)
			{
				if(_skipFrame)
				{
					AGSCON::Graphics::SkipFrame();
					_skipFrame = false;
					return;
				}

				//TODO - begin and end semantics mixed up. not sure about this yet.
				AGSCON::Graphics::BeginFrame();
				_render(clearDrawListAfterwards);
				AGSCON::Graphics::EndFrame();

				//age the contents of the pool to purge stale stuff
				//anything older than ~0.2 seconds is assumed not likely to be reused soon
				//ideally this would be pinged whenever the scene reloads (room change) so we could purge everything
				//we want to avoid recreating resources for stuff that happens in cycles, but right now we can't handle very long cycles (else we get it confused with room changes)
				//so for now i have it set to a small value which is not likely to exceed room fade-out time
				//also note: this is done after EndFrame to ensure textures have finished rendering (and vertex buffers are done being used)
				for(int i=0;i<(int)ddbitmaps_pool.size();i++)
				{
					DDBitmap* ddb = ddbitmaps_pool[i];
					ddb->_aging++;
					if(ddb->_aging>6)
					{
						std::swap(ddbitmaps_pool[ddbitmaps_pool.size()-1],ddbitmaps_pool[i]);
						ddbitmaps_pool.resize(ddbitmaps_pool.size()-1);
						delete ddb;
					}
				}
			}

			void ConsoleGraphicsDriver::_render(bool clearDrawListAfterwards)
			{
				//TODO MBG - READ THIS CAREFULLY!

				//REMINDER: viewport was preserved. why?
				//D3DVIEWPORT9 pViewport;
				//direct3ddevice->GetViewport(&pViewport);

				AGSCON::Graphics::BeginRender();

                static AGSCON::Graphics::RenderTarget* rtPrescale;
                //NEW
				if (!_renderSprAtScreenRes)
					AGSCON::Graphics::BindRenderTarget(0, pNativeSurface);
                else
                {
                //HACKS! NOT EVEN TRYING RIGHT NOW!
                    if(!rtPrescale)
                        rtPrescale = AGSCON::Graphics::RenderTarget_Create(800, 720);
                  AGSCON::Graphics::BindRenderTarget(0, rtPrescale);
                    AGSCON::Graphics::SetViewport(0,0,800,720);
                }


				//OLD
				//TODO - proper offscreen stuff
				//if (!_renderSprAtScreenRes)
				//	AGSCON::Graphics::BindRenderTarget(0, pNativeSurface);
				//AGSCON::Graphics::BindBackbufferRenderTarget();

				////note the odd choice of alpha
				AGSCON::Graphics::ClearColor(0,0,0,128);

				// "if showing at 2x size, the sprite can get distorted otherwise"
				//set sampler to CLAMP

				RenderSpriteBatches();

				if (_renderSprAtScreenRes)
                {
                    AGSCON::Graphics::Rectangle r;
                    r.left = 0;
                    r.right = 800;
                    r.top = 0;
                    r.bottom = 720;
                    AGSCON::Graphics::PresentNative(rtPrescale, &r);
                }
                else
                {
					//TODO - uhhhh I guess this is the final presentation logic? that's pretty shoddy. need to re-engineer that
					

					// "use correct sampling method when stretching buffer to the final rect"
					// it seems like this is a weak approximation of the intended real filtering capability
					// for now I'll just ignore it and hardcode it by game
					//_filter->SetSamplerStateForStandardSprite();


					//D3DTEXTUREFILTERTYPE filterType;
					//direct3ddevice->GetSamplerState(0, D3DSAMP_MAGFILTER, (DWORD*)&filterType);
					//if (direct3ddevice->StretchRect(pNativeSurface, NULL, pBackBuffer, &viewport_rect, filterType) != D3D_OK)
					//{
					//	throw Ali3DException("IDirect3DSurface9::StretchRect failed");
					//}
					//direct3ddevice->SetViewport(&pViewport);

					AGSCON::Graphics::PresentNative(pNativeSurface, &viewport_rect);
				} 

				//not right place for this... but for now...
				AGSCON::Graphics::EndRender();

				if (clearDrawListAfterwards)
				{
					BackupDrawLists();
					ClearDrawLists();
				}
				ResetFxPool();
			}

			void ConsoleGraphicsDriver::RenderSpriteBatches()
			{
				// Render all the sprite batches with necessary transformations
				for (size_t i = 0; i <= _actSpriteBatch; ++i)
				{
					const Rect &viewport = _spriteBatches[i].Viewport;
					const DDSpriteBatch &batch = _spriteBatches[i];
					if (!viewport.IsEmpty())
					{
						AGSCON::Graphics::Rectangle scissor;
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
						//TODO - NOT RIGHT
						//AGSCON::Graphics::SetScissor(scissor.left, scissor.top, scissor.right - scissor.left, scissor.bottom - scissor.top);
					}
					else
					{
						AGSCON::Graphics::ClearScissor();
					}
					_stageVirtualScreen = GetStageScreen(i);
					RenderSpriteBatch(batch);
				}

				_stageVirtualScreen = GetStageScreen(0);
				AGSCON::Graphics::ClearScissor();
			}

			void ConsoleGraphicsDriver::RenderSpriteBatch(const DDSpriteBatch &batch)
			{
				DDDrawListEntry stageEntry; // raw-draw plugin support

				const std::vector<DDDrawListEntry> &listToDraw = batch.List;
				for (size_t i = 0; i < listToDraw.size(); ++i)
				{
					if (listToDraw[i].skip)
						continue;

					const DDDrawListEntry *sprite = &listToDraw[i];
					if (listToDraw[i].bitmap == NULL)
					{
						//MBG - WTF IS THIS CAST HERE?
						//if (DoNullSpriteCallback(listToDraw[i].x, (int)direct3ddevice))
						if (DoNullSpriteCallback(listToDraw[i].x, listToDraw[i].y)) //GUESSS?
							stageEntry = DDDrawListEntry((DDBitmap*)_stageVirtualScreenDDB);
						else
							continue;
						sprite = &stageEntry;
					}

					this->_renderSprite(sprite, batch.Matrix);
				}
			}

			void ConsoleGraphicsDriver::InitSpriteBatch(size_t index, const SpriteBatchDesc &desc)
			{
				if (_spriteBatches.size() <= index)
					_spriteBatches.resize(index + 1);
				_spriteBatches[index].List.clear();

				Rect viewport = desc.Viewport;
				float scaled_offx = (_srcRect.GetWidth() - desc.Transform.ScaleX * (float)_srcRect.GetWidth()) / 2.f;
				float scaled_offy = (_srcRect.GetHeight() - desc.Transform.ScaleY * (float)_srcRect.GetHeight()) / 2.f;
				MatrixTransform2D(_spriteBatches[index].Matrix,
					desc.Transform.X + viewport.Left - scaled_offx, -(desc.Transform.Y + viewport.Top - scaled_offy),
					desc.Transform.ScaleX, desc.Transform.ScaleY, desc.Transform.Rotate);

				//TODO RATA - crap.
				//// Then apply global node transformation (flip and offset)
				//int node_tx = desc.Offset.X, node_ty = desc.Offset.Y;
				//float node_sx = 1.f, node_sy = 1.f;
				//if ((desc.Flip == kFlip_Vertical) || (desc.Flip == kFlip_Both))
				//{
				//	int left = _srcRect.GetWidth() - (viewport.Right + 1);
				//	viewport.MoveToX(left);
				//	node_sx = -1.f;
				//}
				//if ((desc.Flip == kFlip_Horizontal) || (desc.Flip == kFlip_Both))
				//{
				//	int top = _srcRect.GetHeight() - (viewport.Bottom + 1);
				//	viewport.MoveToY(top);
				//	node_sy = -1.f;
				//}
				//viewport = Rect::MoveBy(viewport, node_tx, node_ty);
				//D3DMATRIX matFlip;
				//MatrixTransform2D(matFlip, node_tx, -(node_ty), node_sx, node_sy, 0.f);
				//MatrixMultiply(_spriteBatches[index].Matrix, matViewportFinal, matFlip);
				//_spriteBatches[index].Viewport = viewport;

				// create stage screen for plugin raw drawing
				int src_w = viewport.GetWidth() / desc.Transform.ScaleX;
				int src_h = viewport.GetHeight() / desc.Transform.ScaleY;
				CreateStageScreen(index, Size(src_w, src_h));
			}

			void ConsoleGraphicsDriver::ResetAllBatches()
			{
				for (size_t i = 0; i < _spriteBatches.size(); ++i)
					_spriteBatches[i].List.clear();
			}

			void ConsoleGraphicsDriver::ClearDrawBackups()
			{
				_backupBatchDescs.clear();
				_backupBatches.clear();
			}

			void ConsoleGraphicsDriver::BackupDrawLists()
			{
				ClearDrawBackups();
				for (size_t i = 0; i <= _actSpriteBatch; ++i)
				{
					_backupBatchDescs.push_back(_spriteBatchDesc[i]);
					_backupBatches.push_back(_spriteBatches[i]);
				}
			}

			void ConsoleGraphicsDriver::RestoreDrawLists()
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

			void ConsoleGraphicsDriver::DrawSprite(int x, int y, IDriverDependantBitmap* bitmap)
			{
				_spriteBatches[_actSpriteBatch].List.push_back(DDDrawListEntry((DDBitmap*)bitmap, x, y));
			}

			void ConsoleGraphicsDriver::DestroyDDB(IDriverDependantBitmap* bitmap)
			{
				// Remove deleted DDB from backups
				for (DDSpriteBatches::iterator it = _backupBatches.begin(); it != _backupBatches.end(); ++it)
				{
					std::vector<DDDrawListEntry> &drawlist = it->List;
					for (size_t i = 0; i < drawlist.size(); i++)
					{
						if (drawlist[i].bitmap == bitmap)
							drawlist[i].skip = true;
					}
				}

				ddbitmaps_pool.push_back((DDBitmap*)bitmap);
			}

			void ConsoleGraphicsDriver::UpdateTextureRegion(DDTextureTile *tile, Bitmap *bitmap, DDBitmap *target, bool hasAlpha)
			{
				bool usingLinearFiltering = _filter->NeedToColourEdgeLines();
				if(!AGSCON::Graphics::BitmapToVideoMem(target,tile,bitmap,hasAlpha,usingLinearFiltering))
				{
					AGSCON::Graphics::TextureLock textureLock;
					AGSCON::Graphics::Texture_Lock(tile->texture, &textureLock);
					//TODO RATA -- OLD WORKING WAY
					//BitmapToVideoMem(bitmap, hasAlpha, tile, target, (char*)textureLock.Ptr, textureLock.StrideBytes, usingLinearFiltering);
					//TODO RATA - MAKE WORK THIS WAY
					//if (target->_opaque)
					//	BitmapToVideoMemOpaque(bitmap, hasAlpha, tile, target, memPtr, lockedRegion.Pitch);
					//else
					//	BitmapToVideoMem(bitmap, hasAlpha, tile, target, memPtr, lockedRegion.Pitch, usingLinearFiltering);
					AGSCON::Graphics::Texture_Unlock(tile->texture, &textureLock);
				}
			}

			void ConsoleGraphicsDriver::UpdateDDBFromBitmap(IDriverDependantBitmap* bitmapToUpdate, Bitmap *bitmap, bool hasAlpha)
			{
				DDBitmap *target = (DDBitmap*)bitmapToUpdate;
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

			int ConsoleGraphicsDriver::GetCompatibleBitmapFormat(int color_depth)
			{
				if (color_depth == 8)
					return 8;
				if (color_depth > 8 && color_depth <= 16)
					return 16;
				return 32;
			}

			void ConsoleGraphicsDriver::AdjustSizeToNearestSupportedByCard(int *width, int *height)
			{
				int allocatedWidth = *width, allocatedHeight = *height;

				AGSCON::Graphics::ImageRequest req;
				AGSCON::Graphics::ImageReport report;
				req.Width = *width;
				req.Height = *height;

				AGSCON::Graphics::MakeImageRequest(&req, &report);

				//eventually we should keep this full functionality on consoles as there are annoying restrictions
				//at surprising times for various texture formats without pow2 or odd strides; also minimum texture sizes can strike

				//(need to make API for this tho)

				if (report.MustNPOT)
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

				*width = allocatedWidth;
				*height = allocatedHeight;
			}

			DDBitmap* ConsoleGraphicsDriver::GetPooledDDB(Bitmap *bitmap, bool hasAlpha, bool opaque)
			{
				for(int i=0;i<(int)ddbitmaps_pool.size();i++)
				{
					DDBitmap* ddb = ddbitmaps_pool[i];
					bool match = true;
					match &= ddb->GetWidth() == bitmap->GetWidth();
					match &= ddb->GetHeight() == bitmap->GetHeight();
					match &= ddb->GetColorDepth() == bitmap->GetColorDepth();
					match &= ddb->GetOpaque() == opaque;
					if(match)
					{
						ddbitmaps_pool.erase(ddbitmaps_pool.begin()+i);
						ddb->Recycle();
						return ddb;
					}
				}
				return nullptr;
			}

			IDriverDependantBitmap* ConsoleGraphicsDriver::CreateDDBFromBitmap(Bitmap *bitmap, bool hasAlpha, bool opaque)
			{
				int allocatedWidth = bitmap->GetWidth();
				int allocatedHeight = bitmap->GetHeight();
				if (bitmap->GetColorDepth() != GetCompatibleBitmapFormat(bitmap->GetColorDepth()))
					throw Ali3DException("CreateDDBFromBitmap: bitmap colour depth not supported");
				int colorDepth = bitmap->GetColorDepth();

				//check pool for anything we can reuse
				DDBitmap* ddb = GetPooledDDB(bitmap, hasAlpha,opaque);
				if(!ddb)
				{
					ddb = new DDBitmap(bitmap->GetWidth(), bitmap->GetHeight(), colorDepth, opaque);

					AdjustSizeToNearestSupportedByCard(&allocatedWidth, &allocatedHeight);
					int tilesAcross = 1, tilesDown = 1;

					AGSCON::Graphics::ImageRequest req;
					AGSCON::Graphics::ImageReport report;
					req.Width = allocatedWidth;
					req.Height = allocatedHeight;

					AGSCON::Graphics::MakeImageRequest(&req, &report);

					int maxWidth = report.MaxWidth;
					int maxHeight = report.MaxHeight;

					//maxWidth = 64; //TEST: this is just for testing. why not?
					//maxHeight = 64; //TEST: this is just for testing. why not?

					// Calculate how many textures will be necessary to
					// store this image
					tilesAcross = (allocatedWidth + maxWidth - 1) / maxWidth;
					tilesDown = (allocatedHeight + maxHeight - 1) / maxHeight;
					int tileWidth = bitmap->GetWidth() / tilesAcross;
					int lastTileExtraWidth = bitmap->GetWidth() % tilesAcross;
					int tileHeight = bitmap->GetHeight() / tilesDown;
					int lastTileExtraHeight = bitmap->GetHeight() % tilesDown;
					int tileAllocatedWidth = tileWidth;
					int tileAllocatedHeight = tileHeight;

					AdjustSizeToNearestSupportedByCard(&tileAllocatedWidth, &tileAllocatedHeight);

					int numTiles = tilesAcross * tilesDown;
					DDTextureTile *tiles = (DDTextureTile*)malloc(sizeof(DDTextureTile) * numTiles);
					memset(tiles, 0, sizeof(DDTextureTile) * numTiles);

					COOLCUSTOMVERTEX *vertices = new COOLCUSTOMVERTEX[numTiles*4];

					for (int x = 0; x < tilesAcross; x++)
					{
						for (int y = 0; y < tilesDown; y++)
						{
							DDTextureTile *thisTile = &tiles[y * tilesAcross + x];
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

							thisTile->texture = AGSCON::Graphics::Texture_Create(thisAllocatedWidth, thisAllocatedHeight);
						}
					}

					ddb->_vertex = AGSCON::Graphics::VertexBuffer_Create(AGSCON::Graphics::shaders.standardVertexLayout, numTiles*4, vertices);

					ddb->_numTiles = numTiles;
					ddb->_tiles = tiles;
				}

				UpdateDDBFromBitmap(ddb, bitmap, hasAlpha);

				return ddb;
			}

			void ConsoleGraphicsDriver::do_fade(bool fadingOut, int speed, int targetColourRed, int targetColourGreen, int targetColourBlue)
			{
				//YUCK!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

				if (fadingOut)
				{
					this->_reDrawLastFrame();
				}
				else if (_drawScreenCallback != NULL)
					_drawScreenCallback();

				Bitmap *blackSquare = BitmapHelper::CreateBitmap(16, 16, 32);
				blackSquare->Clear(makecol32(targetColourRed, targetColourGreen, targetColourBlue));
				IDriverDependantBitmap *ddb = this->CreateDDBFromBitmap(blackSquare, false, true);
				delete blackSquare;

				BeginSpriteBatch(_srcRect, SpriteTransform());
				ddb->SetStretch(_srcRect.GetWidth(), _srcRect.GetHeight(), false);
				// NOTE: what happens here is that we are trying to prevent global offset to be applied to this sprite :/
				this->DrawSprite(-_globalViewOff.X, -_globalViewOff.Y, ddb);

				if (speed <= 0) speed = 16;
				speed *= 2;  // harmonise speeds with software driver which is faster

				int timerValue = *_loopTimer;
				for(;;)
				{
					int elapsed = *_loopTimer - timerValue;
					int a = speed * elapsed + 1;
					if(a>255) break;

					ddb->SetTransparency(fadingOut ? a : (255 - a));
					this->_renderAndPresent(false);
					update_polled_mp3();
				}

				if (fadingOut)
				{
					ddb->SetTransparency(0);
					this->_renderAndPresent(false);
				}

				this->DestroyDDB(ddb);
				this->ClearDrawLists();
				ResetFxPool();
			}

			void ConsoleGraphicsDriver::FadeOut(int speed, int targetColourRed, int targetColourGreen, int targetColourBlue) 
			{
				do_fade(true, speed, targetColourRed, targetColourGreen, targetColourBlue);
			}

			void ConsoleGraphicsDriver::FadeIn(int speed, PALLETE p, int targetColourRed, int targetColourGreen, int targetColourBlue) 
			{
				do_fade(false, speed, targetColourRed, targetColourGreen, targetColourBlue);
			}

			void ConsoleGraphicsDriver::BoxOutEffect(bool blackingOut, int speed, int delay)
			{
				//YUCK!!!!!!!!!!!!!!!

				if (blackingOut)
				{
					this->_reDrawLastFrame();
				}
				else if (_drawScreenCallback != NULL)
					_drawScreenCallback();

				Bitmap *blackSquare = BitmapHelper::CreateBitmap(16, 16, 32);
				blackSquare->Clear();
				IDriverDependantBitmap *d3db = this->CreateDDBFromBitmap(blackSquare, false, true);
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
					DDSpriteBatch &batch = _spriteBatches.back();
					std::vector<DDDrawListEntry> &drawList = batch.List;
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

					this->_renderAndPresent(false);

					if (_pollingCallback)
						_pollingCallback();
					platform->Delay(delay);
				}

				this->DestroyDDB(d3db);
				this->ClearDrawLists();
				ResetFxPool();
			}

			bool ConsoleGraphicsDriver::PlayVideo(const char *filename, bool useAVISound, VideoSkipType skipType, bool stretchToFullScreen)
			{
				return false;
			}

			void ConsoleGraphicsDriver::SetScreenFade(int red, int green, int blue)
			{
				//TODO RATA
				//D3DBitmap *ddb = static_cast<D3DBitmap*>(MakeFx(red, green, blue));
				//ddb->SetStretch(_spriteBatches[_actSpriteBatch].Viewport.GetWidth(),
				//	_spriteBatches[_actSpriteBatch].Viewport.GetHeight(), false);
				//ddb->SetTransparency(0);
				//_spriteBatches[_actSpriteBatch].List.push_back(D3DDrawListEntry(ddb));
			}

			void ConsoleGraphicsDriver::SetScreenTint(int red, int green, int blue)
			{ 
				//TODO RATA
				//if (red == 0 && green == 0 && blue == 0) return;
				//D3DBitmap *ddb = static_cast<D3DBitmap*>(MakeFx(red, green, blue));
				//ddb->SetStretch(_spriteBatches[_actSpriteBatch].Viewport.GetWidth(),
				//	_spriteBatches[_actSpriteBatch].Viewport.GetHeight(), false);
				//ddb->SetTransparency(128);
				//_spriteBatches[_actSpriteBatch].List.push_back(D3DDrawListEntry(ddb));
			}

			static class ConsoleGraphicsFactory : public GfxDriverFactoryBase<ConsoleGraphicsDriver, ConsoleGfxFilter>
			{
			public:
				virtual size_t               GetFilterCount() const { return 2; }
				virtual const GfxFilterInfo *GetFilterInfo(size_t index) const
				{
					switch (index)
					{
						case 0:
							return &ConsoleGfxFilter::FilterInfo;
						case 1:
							return &AAConsoleGfxFilter::FilterInfo;
						default:
							return NULL;
					}
				}
				virtual String               GetDefaultFilterID() const { return ConsoleGfxFilter::FilterInfo.Id; }

				virtual ~ConsoleGraphicsFactory()
				{
				}


			private:


				virtual void Shutdown()
				{
					DestroyDriver();
					this->~ConsoleGraphicsFactory();
				}

				virtual void DestroyDriver()
				{
					s_inst.~ConsoleGraphicsDriver();
					_driver = nullptr;
				}

				virtual ConsoleGraphicsDriver   *EnsureDriverCreated() {
					s_inst.Init();
					return &s_inst;
				}

				virtual ConsoleGfxFilter        *CreateFilter(const String &id)
				{
					if (ConsoleGfxFilter::FilterInfo.Id.CompareNoCase(id) == 0)
						return new ConsoleGfxFilter();
					else if (AAConsoleGfxFilter::FilterInfo.Id.CompareNoCase(id) == 0)
						return new AAConsoleGfxFilter();
					return NULL;
				}

				bool Init();
			} s_ConsoleGraphicsFactory;
			ConsoleGraphicsFactory* g_pGraphicsFactory = &s_ConsoleGraphicsFactory;

		} // namespace AL3DConsole
	} // namespace Engine
} // namespace AGS
