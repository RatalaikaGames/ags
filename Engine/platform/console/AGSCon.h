#pragma once

namespace AGSCON
{
	//This API represents a tried and true set of minimal graphics APIs suitable for porting across all current-gen consoles
	//That is, assuming a certain simplified 2d game model.
	//No big choices here are arbitrary; they are all significant.
	namespace Graphics
	{
		class Texture;
		class VertexBuffer;
		class Program;
		class RenderTarget;
		class VertexLayout;
		class Sampler;
		class UniformLocation;

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

		void MakeImageRequest(const ImageRequest* request, ImageReport* report);

		void BeginFrame();
		void BeginRender();
		void EndRender();
		void EndFrame();

		Texture* Texture_Create(int w, int h);
		void Texture_Lock(Texture* texture, TextureLock* lockData);
		void Texture_Unlock(Texture* texture, TextureLock* lockData);
		void Texture_Destroy(Texture* tex);

		VertexLayout* VertexLayout_Create(const VertexLayoutDescr* layoutDescr);
		void VertexLayout_Destroy(VertexLayout* layout);

		VertexBuffer* VertexBuffer_Create(const VertexLayout* layout, int nElements, void* data);
		void VertexBuffer_Destroy(VertexBuffer* vb);

		//This takes an enum instead of a buffer or a path because the details of pathing and resource loading are up to the console backend implementation
		Program* Program_Create(VertexLayout* layout, ProgramType which);
		void Program_Destroy(Program* program);

		UniformLocation* UniformLocation_Create(Program* program, const char* name);
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

		//Sets the current render target
		//AND sets the viewport to match it
		//AND clears the scissor
		void SetRenderTarget(int index, RenderTarget* rt);

		void SetBackbufferRenderTarget();
		
		//Clears the currently set RT's color buffer only
		void ClearColor(int red, int green, int blue, int alpha);

		void BindFragmentTexture(int target, Texture* texture, Sampler* sampler);
		void BindVertexBuffer(VertexBuffer* vb);
		void BindProgram(Program* program);

		void UniformFloat2(const UniformLocation* uniformLocation, const float* data);
		void UniformFloat4(const UniformLocation* uniformLocation, const float* data);
		void UniformMatrix44(const UniformLocation* uniformLocation, const float* data);

		void DrawVertices(int start, int count, PrimitiveType primitiveType);
	}
}