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

#pragma once

#include <allegro.h>
#include "gfx/bitmap.h"
#include "gfx/ddb.h"
#include "gfx/gfxdriverfactorybase.h"
#include "gfx/gfxdriverbase.h"
#include "util/library.h"
#include "util/string.h"
#include "platform/console/AGSCon.h"


namespace AGS
{
	namespace Engine
	{
		namespace AL3DConsole
		{
			using AGS::Common::Bitmap;
			using AGS::Common::String;
			class ConsoleGfxFilter;

			struct Matrix44 {
				union {
					struct {
						float        _11, _12, _13, _14;
						float        _21, _22, _23, _24;
						float        _31, _32, _33, _34;
						float        _41, _42, _43, _44;

					};
					float m[4][4];
				};
			};

			struct Vector2f {
				float x;
				float y;
			};

			//I think the premise here is that some images may be too large for texture
			//I suppose in case of whole giant maps done as one image, for instance
			//I judge this is plausible even on a modern system, so I'd better support it
			//Key point is, it isn't for performance, but for compatibility.
			//Worst case will be several textures, not 100s
			struct DDTextureTile : public TextureTile
			{
				AGSCON::Graphics::Texture* texture;
			};

			class DDBitmap : public VideoMemDDB
			{
			public:
				// Transparency is a bit counter-intuitive
				// 0=not transparent, 255=invisible, 1..254 barely visible .. mostly visible
				virtual void SetTransparency(int transparency) { _transparency = transparency; }
				virtual void SetFlippedLeftRight(bool isFlipped) { _flipped = isFlipped; }
				virtual void SetStretch(int width, int height, bool useResampler = true)
				{
					_stretchToWidth = width;
					_stretchToHeight = height;
					_useResampler = useResampler;
				}
				virtual void SetLightLevel(int lightLevel)  { _lightLevel = lightLevel; }
				virtual void SetTint(int red, int green, int blue, int tintSaturation) 
				{
					_red = red;
					_green = green;
					_blue = blue;
					_tintSaturation = tintSaturation;
				}

				bool _flipped;
				int _stretchToWidth, _stretchToHeight;
				bool _useResampler;
				int _red, _green, _blue;
				int _tintSaturation;
				int _lightLevel;
				bool _hasAlpha;
				int _transparency;
				int _aging;
				AGSCON::Graphics::VertexBuffer* _vertex;

				DDTextureTile *_tiles;
				int _numTiles;

				DDBitmap(int width, int height, int colDepth, bool opaque)
				{
					_width = width;
					_height = height;
					_colDepth = colDepth;
					_opaque = opaque;
					_vertex = NULL;
					_tiles = NULL;
					_numTiles = 0;
					Recycle();
				}

				void Recycle()
				{
					_aging = 0;
					_flipped = false;
					_hasAlpha = false;
					_stretchToWidth = 0;
					_stretchToHeight = 0;
					_useResampler = false;
					_tintSaturation = 0;
					_lightLevel = 0;
					_transparency = 0;
				}

				int GetWidthToRender() { return (_stretchToWidth > 0) ? _stretchToWidth : _width; }
				int GetHeightToRender() { return (_stretchToHeight > 0) ? _stretchToHeight : _height; }

				bool GetOpaque() const { return _opaque; }

				void Dispose();

				~DDBitmap()
				{
					Dispose();
				}
			};

			struct COOLCUSTOMVERTEX
			{
				Vector2f   position; // The position.
				float       tu, tv;   // The texture coordinates.
			};

			typedef SpriteDrawListEntry<DDBitmap> DDDrawListEntry;
			// DD renderer's sprite batch
			struct DDSpriteBatch
			{
				// List of sprites to render
				std::vector<DDDrawListEntry> List;
				// Transformation matrix, built from the batch description
				Matrix44 Matrix;
			};
			typedef std::vector<DDSpriteBatch>    DDSpriteBatches;


			class ConsoleGraphicsDriver : public VideoMemoryGraphicsDriver
			{
			public:
				ConsoleGraphicsDriver(int secret, float key);

				virtual const char*GetDriverName() { return "Console"; }
				virtual const char*GetDriverID() { return "CONS"; }
				virtual void SetTintMethod(TintMethod method);
				virtual bool SetDisplayMode(const DisplayMode &mode, volatile int *loopTimer);
				virtual bool SetNativeSize(const Size &src_size);
				virtual bool SetRenderFrame(const Rect &dst_rect);
				virtual int  GetDisplayDepthForNativeDepth(int native_color_depth) const;
				virtual IGfxModeList *GetSupportedModeList(int color_depth);
				virtual bool IsModeSupported(const DisplayMode &mode);
				virtual PGfxFilter GetGraphicsFilter() const;
				virtual void UnInit();
				virtual void ClearRectangle(int x1, int y1, int x2, int y2, RGB *colorToUse);
				virtual int  GetCompatibleBitmapFormat(int color_depth);
				virtual IDriverDependantBitmap* CreateDDBFromBitmap(Bitmap *bitmap, bool hasAlpha, bool opaque);
				virtual void UpdateDDBFromBitmap(IDriverDependantBitmap* bitmapToUpdate, Bitmap *bitmap, bool hasAlpha);
				virtual void DestroyDDB(IDriverDependantBitmap* bitmap);
				virtual void DrawSprite(int x, int y, IDriverDependantBitmap* bitmap);
				virtual void RenderToBackBuffer();
				virtual void Render();
				virtual void Render(GlobalFlipType flip);
				virtual bool GetCopyOfScreenIntoBitmap(Bitmap *destination, bool at_native_res, GraphicResolution *want_fmt);
				virtual void EnableVsyncBeforeRender(bool enabled) { }
				virtual void Vsync();
				virtual void RenderSpritesAtScreenResolution(bool enabled, int supersampling) { _renderSprAtScreenRes = enabled; };
				virtual void FadeOut(int speed, int targetColourRed, int targetColourGreen, int targetColourBlue);
				virtual void FadeIn(int speed, PALETTE p, int targetColourRed, int targetColourGreen, int targetColourBlue);
				virtual void BoxOutEffect(bool blackingOut, int speed, int delay);
				virtual bool PlayVideo(const char *filename, bool useSound, VideoSkipType skipType, bool stretchToFullScreen);
				virtual bool SupportsGammaControl();
				virtual void SetGamma(int newGamma);
				virtual void UseSmoothScaling(bool enabled) { _smoothScaling = enabled; }
				virtual bool RequiresFullRedrawEachFrame() { return true; }
				virtual bool HasAcceleratedTransform() { return true; }
				virtual void SetScreenTint(int red, int green, int blue);

				typedef std::shared_ptr<ConsoleGfxFilter> PConsoleFilter;

				void SetGraphicsFilter(PConsoleFilter filter);

				ConsoleGraphicsDriver();
				virtual ~ConsoleGraphicsDriver();


			private:

				friend class ConsoleGraphicsFactory;

				void Init();
				DDBitmap* GetPooledDDB(Bitmap *bitmap, bool hasAlpha, bool opaque);

				bool _initialized;
				PConsoleFilter _filter;

				AGSCON::Graphics::RenderTarget* pNativeSurface;
				AGSCON::Graphics::Rectangle viewport_rect;
				int _tint_red, _tint_green, _tint_blue;
				COOLCUSTOMVERTEX defaultVertices[4];
				String previousError;

				bool _smoothScaling;
				bool _legacyPixelShader;
				float _pixelRenderXOffset;
				float _pixelRenderYOffset;
				bool _renderSprAtScreenRes;

				// TODO: find a way to have this tint sprite in the normal sprite list (or use shader instead!)
				Bitmap *_screenTintLayer;
				DDBitmap* _screenTintLayerDDB;
				DDDrawListEntry _screenTintSprite;

				Matrix44 currentProjection;

				DDSpriteBatches _spriteBatches;
				GlobalFlipType flipTypeLastTime;
				// TODO: these draw list backups are needed only for the fade-in/out effects
				// find out if it's possible to reimplement these effects in main drawing routine.
				SpriteBatchDescs _backupBatchDescs;
				DDSpriteBatches _backupBatches;

				//resource pooling
				std::vector<DDBitmap*> ddbitmaps_pool;

				// Called after new mode was successfully initialized
				virtual void InitSpriteBatch(size_t index, const SpriteBatchDesc &desc);
				virtual void ResetAllBatches();
				// Called when the direct3d device is created for the first time
				void InitializeDDState();
				void SetupViewport();
				// Unset parameters and release resources related to the display mode
				void AdjustSizeToNearestSupportedByCard(int *width, int *height);
				void UpdateTextureRegion(DDTextureTile *tile, Bitmap *bitmap, DDBitmap *target, bool hasAlpha);
				void CreateVirtualScreen();
				void do_fade(bool fadingOut, int speed, int targetColourRed, int targetColourGreen, int targetColourBlue);
				void create_screen_tint_bitmap();
				// Backup all draw lists in the temp storage
				void BackupDrawLists();
				// Restore draw lists from the temp storage
				void RestoreDrawLists();
				// Deletes draw list backups
				void ClearDrawBackups();
				void _renderAndPresent(GlobalFlipType flip, bool clearDrawListAfterwards);
				void _render(GlobalFlipType flip, bool clearDrawListAfterwards);
				void _reDrawLastFrame();
				void RenderSpriteBatches(GlobalFlipType flip);
				void RenderSpriteBatch(const DDSpriteBatch &batch, GlobalFlipType flip);
				void _renderSprite(const DDDrawListEntry *entry, const Matrix44 &matGlobal, bool globalLeftRightFlip, bool globalTopBottomFlip);
			};


		} // namespace AL3DConsole
	} // namespace Engine
} // namespace AGS

