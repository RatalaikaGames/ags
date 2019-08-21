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
// This API represents a tried and true set of minimal APIs suitable for porting across all current-gen consoles
// That is, assuming a certain simplified 2d game model.
//
//=============================================================================

#ifndef __AGS_EE_PLATFORM__AGSCON_H
#define __AGS_EE_PLATFORM__AGSCON_H

class TextureTile;

namespace AGS
{
	namespace Common
	{
		class Bitmap;
	}
	namespace Engine
	{
		namespace AL3DConsole
		{
			struct DDTextureTile;
			class DDBitmap;
		}
	}
}

namespace AGSCON
{
	bool CreateDirectory(const char* path);
	const char* SetCurrentDirectory(const char* path);
	const char* GetCurrentDirectory();

	namespace Graphics
	{
		class Texture;
		class VertexBuffer;
		class Program;
		class RenderTarget;
		class VertexLayout;
		class Sampler;
		class UniformLocation;

		struct Rectangle
		{
			int left, top, right, bottom;
		};

		enum class PrimitiveType
		{
			TriangleList,
			TriangleStrip,
			TriangleFan
		};

		enum class ShaderStage
		{
			Vertex = 0,
			Fragment = 1,
		};

		//the format of a vertex element (aka attribute)
		enum class AttributeFormat
		{
			Float
		};

		struct TextureLock
		{
			void* Ptr;
			int StrideBytes;
		};

		//one element of a vertex (aka attribute)
		struct VertexLayoutDescr_Attribute
		{
			const char* Name;
			int Offset;
			AttributeFormat Format;
			int Size; //i.e. 1,2,3, or 4
		};

		struct VertexLayoutDescr
		{
			int Stride;
			int NumAttributes;
			const VertexLayoutDescr_Attribute* Attributes;
		};

		//Asks the backend for information about how to handle a given image
		struct ImageRequest
		{
			int Width, Height;
		};

		//The resulting report associated with a ImageRequest
		struct ImageReport
		{
			//The request that was made
			ImageRequest Request;

			//The maximum width possible for a this image
			int MaxWidth;

			//The maximum height possible for this image
			int MaxHeight;

			//Whether this image must be power of 2
			bool MustNPOT;
		};

		enum class SamplerMinFilter
		{
			Nearest,
			Linear
		};

		enum class SamplerMagFilter
		{
			Nearest,
			Linear
		};

		enum class SamplerWrapMode
		{
			Clamp,
			Repeat
		};

		enum class ProgramType
		{
			Standard,
			Tint,
			TintLegacy
		};

		struct SamplerDescr
		{
			SamplerMinFilter MinFilter;
			SamplerMagFilter MagFilter;
			SamplerWrapMode WrapModeS;
			SamplerWrapMode WrapModeT;
		};

		void Initialize();
		const int* GetColorShifts();

		void MakeImageRequest(const ImageRequest* request, ImageReport* report);

		void BeginFrame();
		void BeginRender();
		void EndRender();
		void EndFrame();

		Texture* Texture_Create(int w, int h);
		void Texture_Lock(Texture* texture, TextureLock* lockData);
		void Texture_Unlock(Texture* texture, TextureLock* lockData);
		void Texture_Destroy(Texture* tex);

		//Tries to let the backend import the video memory
		//This is important, in case the video memory can be used directly (to save time converting the format and uploading the data)
		//It may be DIFFICULT to use it directly, but it can be done... I do it!
		//You can also copy the data more efficiently in here if you know the format allegro's storing it in.
		//Since that's probably a linear format, it's an easy optimization...
		bool BitmapToVideoMem(AGS::Engine::AL3DConsole::DDBitmap *target, AGS::Engine::AL3DConsole::DDTextureTile *tile, AGS::Common::Bitmap *bitmap, bool hasAlpha, bool usingLinearFiltering);

		VertexLayout* VertexLayout_Create(const VertexLayoutDescr* layoutDescr);
		void VertexLayout_Destroy(VertexLayout* layout);

		VertexBuffer* VertexBuffer_Create(const VertexLayout* layout, int nElements, const void* data);
		void VertexBuffer_Destroy(VertexBuffer* vb);

		//This takes an enum instead of a buffer or a path because the details of pathing and resource loading are up to the console backend implementation
		//Note: I've knowingly kept (and some other things) exposed even though they're used only by the backend
		//that's just in case we need to take the shader loading, etc. out of the backend eventually
		//(in other words, the backend's resource setup is done wholly in terms of publicly-accessible stuff)
		//That might be handy for implementing better scalers in the future
		Program* Program_Create(VertexLayout* layout, ProgramType which);
		void Program_Destroy(Program* program);

		//Creates the uniform location for a given uniform on the program...
		UniformLocation* UniformLocation_Create(Program* program, const char* name);

		//Creates the uniform location for a given TEXTURE on the program
		//It's assumed that samplers and textures are linked (each texture has a corresponding shader)
		UniformLocation* UniformLocation_CreateTexture(Program* program, const char* name);

		//Destroys a UniformLocation
		void UniformLocation_Destroy(UniformLocation* uniformLocation);

		RenderTarget* RenderTarget_Create(int width, int height);
		void RenderTarget_Destroy(RenderTarget* rt);

		Sampler* Sampler_Create(const SamplerDescr* samplerDescr);
		void Sampler_Destroy(Sampler* sampler);

		void SetViewport(int x, int y, int w, int h);
		void SetScissor(int x, int y, int w, int h);

		//sets the scissor to the last-set viewport
		//would be better to not lose track of the viewport (need to update this driver to do that)
		void ClearScissor();

		//Binds the current render target
		//AND sets the viewport to match it
		//AND clears the scissor
		void BindRenderTarget(int index, RenderTarget* rt);

		void BindBackbufferRenderTarget();

		//Clears the currently set RT's color buffer only
		void ClearColor(int red, int green, int blue, int alpha);

		void BindFragmentTexture(UniformLocation* location, Texture* texture, Sampler* sampler, UniformLocation* textureControl);
		void BindVertexBuffer(VertexBuffer* vb);
		void BindProgram(Program* program);

		void UniformFloat2(const UniformLocation* uniformLocation, const float* data);
		void UniformFloat4(const UniformLocation* uniformLocation, const float* data);
		void UniformMatrix44(const UniformLocation* uniformLocation, const float* data);

		void DrawVertices(int start, int count, PrimitiveType primitiveType);

		void SetGamma(int newGamma);

		void PresentNative(RenderTarget* tex, const Rectangle* viewport_rect);
		void RepresentNative();

		//---------------------------------------------------
		
		//now we get into the non-generalized things... stuff specifically for AGS

		extern struct SHADERS {
			struct StandardProgram
			{
				AGSCON::Graphics::Program* program;
				AGSCON::Graphics::UniformLocation* um44Projection;
				AGSCON::Graphics::UniformLocation* um44Modelview;
				AGSCON::Graphics::UniformLocation* tex;
				AGSCON::Graphics::UniformLocation* uTextureControl;
			};

			struct STANDARD : public StandardProgram {
				AGSCON::Graphics::UniformLocation* uTextureFactor;
				AGSCON::Graphics::UniformLocation* uControl;
			} standard;

			struct TINT : public StandardProgram {
			} tint;

			struct TINTLEGACY : public StandardProgram {
			} tintLegacy;

			VertexLayout* standardVertexLayout;
			VertexBuffer* quad1x1;

		} shaders;

		extern struct SAMPLERS {
			AGSCON::Graphics::Sampler* nearest;
			AGSCON::Graphics::Sampler* linear;
		} samplers;

	}
}

#endif