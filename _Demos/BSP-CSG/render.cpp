#include "render.h"

float g_texelHalf = 0;
bool g_originBottomLeft = false;

bgfx::VertexDecl PosNormalTangentTexcoordVertex::ms_decl;
bgfx::VertexDecl PosTexCoord0Vertex::ms_decl;
bgfx::VertexDecl DebugVertex::ms_decl;

PosNormalTangentTexcoordVertex s_cubeVertices[24] =
{
	{-1.0f,  1.0f,  1.0f, packF4u( 0.0f,  0.0f,  1.0f), 0,      0,      0 },
	{ 1.0f,  1.0f,  1.0f, packF4u( 0.0f,  0.0f,  1.0f), 0, 0x7fff,      0 },
	{-1.0f, -1.0f,  1.0f, packF4u( 0.0f,  0.0f,  1.0f), 0,      0, 0x7fff },
	{ 1.0f, -1.0f,  1.0f, packF4u( 0.0f,  0.0f,  1.0f), 0, 0x7fff, 0x7fff },
	{-1.0f,  1.0f, -1.0f, packF4u( 0.0f,  0.0f, -1.0f), 0,      0,      0 },
	{ 1.0f,  1.0f, -1.0f, packF4u( 0.0f,  0.0f, -1.0f), 0, 0x7fff,      0 },
	{-1.0f, -1.0f, -1.0f, packF4u( 0.0f,  0.0f, -1.0f), 0,      0, 0x7fff },
	{ 1.0f, -1.0f, -1.0f, packF4u( 0.0f,  0.0f, -1.0f), 0, 0x7fff, 0x7fff },
	{-1.0f,  1.0f,  1.0f, packF4u( 0.0f,  1.0f,  0.0f), 0,      0,      0 },
	{ 1.0f,  1.0f,  1.0f, packF4u( 0.0f,  1.0f,  0.0f), 0, 0x7fff,      0 },
	{-1.0f,  1.0f, -1.0f, packF4u( 0.0f,  1.0f,  0.0f), 0,      0, 0x7fff },
	{ 1.0f,  1.0f, -1.0f, packF4u( 0.0f,  1.0f,  0.0f), 0, 0x7fff, 0x7fff },
	{-1.0f, -1.0f,  1.0f, packF4u( 0.0f, -1.0f,  0.0f), 0,      0,      0 },
	{ 1.0f, -1.0f,  1.0f, packF4u( 0.0f, -1.0f,  0.0f), 0, 0x7fff,      0 },
	{-1.0f, -1.0f, -1.0f, packF4u( 0.0f, -1.0f,  0.0f), 0,      0, 0x7fff },
	{ 1.0f, -1.0f, -1.0f, packF4u( 0.0f, -1.0f,  0.0f), 0, 0x7fff, 0x7fff },
	{ 1.0f, -1.0f,  1.0f, packF4u( 1.0f,  0.0f,  0.0f), 0,      0,      0 },
	{ 1.0f,  1.0f,  1.0f, packF4u( 1.0f,  0.0f,  0.0f), 0, 0x7fff,      0 },
	{ 1.0f, -1.0f, -1.0f, packF4u( 1.0f,  0.0f,  0.0f), 0,      0, 0x7fff },
	{ 1.0f,  1.0f, -1.0f, packF4u( 1.0f,  0.0f,  0.0f), 0, 0x7fff, 0x7fff },
	{-1.0f, -1.0f,  1.0f, packF4u(-1.0f,  0.0f,  0.0f), 0,      0,      0 },
	{-1.0f,  1.0f,  1.0f, packF4u(-1.0f,  0.0f,  0.0f), 0, 0x7fff,      0 },
	{-1.0f, -1.0f, -1.0f, packF4u(-1.0f,  0.0f,  0.0f), 0,      0, 0x7fff },
	{-1.0f,  1.0f, -1.0f, packF4u(-1.0f,  0.0f,  0.0f), 0, 0x7fff, 0x7fff },
};
const uint16_t s_cubeIndices[36] =
{
	 0,  2,  1,
	 1,  2,  3,
	 4,  5,  6,
	 5,  7,  6,

	 8, 10,  9,
	 9, 10, 11,
	12, 13, 14,
	13, 15, 14,

	16, 18, 17,
	17, 18, 19,
	20, 21, 22,
	21, 23, 22,
};

uint32_t packUint32(uint8_t _x, uint8_t _y, uint8_t _z, uint8_t _w)
{
	union
	{
		uint32_t ui32;
		uint8_t arr[4];
	} un;

	un.arr[0] = _x;
	un.arr[1] = _y;
	un.arr[2] = _z;
	un.arr[3] = _w;

	return un.ui32;
}

uint32_t packF4u(float _x, float _y, float _z, float _w)
{
	const uint8_t xx = uint8_t(_x*127.0f + 128.0f);
	const uint8_t yy = uint8_t(_y*127.0f + 128.0f);
	const uint8_t zz = uint8_t(_z*127.0f + 128.0f);
	const uint8_t ww = uint8_t(_w*127.0f + 128.0f);
	return packUint32(xx, yy, zz, ww);
}

void screenSpaceQuad(float _textureWidth, float _textureHeight, float _texelHalf, bool _originBottomLeft, float _width, float _height)
{
	if (bgfx::checkAvailTransientVertexBuffer(3, PosTexCoord0Vertex::ms_decl) )
	{
		bgfx::TransientVertexBuffer vb;
		bgfx::allocTransientVertexBuffer(&vb, 3, PosTexCoord0Vertex::ms_decl);
		PosTexCoord0Vertex* vertex = (PosTexCoord0Vertex*)vb.data;

		const float minx = -_width;
		const float maxx =  _width;
		const float miny = 0.0f;
		const float maxy = _height*2.0f;

		const float texelHalfW = _texelHalf/_textureWidth;
		const float texelHalfH = _texelHalf/_textureHeight;
		const float minu = -1.0f + texelHalfW;
		const float maxu =  1.0f + texelHalfH;

		const float zz = 0.0f;

		float minv = texelHalfH;
		float maxv = 2.0f + texelHalfH;

		if (_originBottomLeft)
		{
			float temp = minv;
			minv = maxv;
			maxv = temp;

			minv -= 1.0f;
			maxv -= 1.0f;
		}

		vertex[0].m_x = minx;
		vertex[0].m_y = miny;
		vertex[0].m_z = zz;
		vertex[0].m_u = minu;
		vertex[0].m_v = minv;

		vertex[1].m_x = maxx;
		vertex[1].m_y = miny;
		vertex[1].m_z = zz;
		vertex[1].m_u = maxu;
		vertex[1].m_v = minv;

		vertex[2].m_x = maxx;
		vertex[2].m_y = maxy;
		vertex[2].m_z = zz;
		vertex[2].m_u = maxu;
		vertex[2].m_v = maxv;

		bgfx::setVertexBuffer(&vb);
	}
}

Renderer::Renderer()
{
}
ERet Renderer::Initialize()
{
	width = 1280;
	height = 720;
	debug = BGFX_DEBUG_TEXT;
	reset = BGFX_RESET_VSYNC;

	bgfx::init();

	// Get renderer capabilities info.
	const bgfx::Caps* caps = bgfx::getCaps();

	if (2 > caps->maxFBAttachments)
	{
		// multiple render targets (MRT) is not supported by GPU
		return ERR_UNSUPPORTED_FEATURE;
	}

	const bgfx::RendererType::Enum renderer = bgfx::getRendererType();
	g_texelHalf = bgfx::RendererType::Direct3D9 == renderer ? 0.5f : 0.0f;
	g_originBottomLeft = bgfx::RendererType::OpenGL == renderer || bgfx::RendererType::OpenGLES == renderer;


	bgfx::reset(width, height, reset);

	// Enable debug text.
	bgfx::setDebug(debug);

	// Set clear color palette for index 0
	bgfx::setClearColor(0, UINT32_C(0x00000000) );

	// Set clear color palette for index 1
	bgfx::setClearColor(1, UINT32_C(0x303030ff) );

	// Set geometry pass view clear state.
	bgfx::setViewClear(RENDER_PASS_GEOMETRY_ID
		, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH
		, 1.0f
		, 0
		, 1
		);

	// Set light pass view clear state.
	bgfx::setViewClear(RENDER_PASS_LIGHT_ID
		, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH
		, 1.0f
		, 0
		, 0
		);

	// Create vertex stream declaration.
	PosNormalTangentTexcoordVertex::init();
	PosTexCoord0Vertex::init();
	DebugVertex::init();

	calcTangents(s_cubeVertices
		, BX_COUNTOF(s_cubeVertices)
		, PosNormalTangentTexcoordVertex::ms_decl
		, s_cubeIndices
		, BX_COUNTOF(s_cubeIndices)
		);

	// Create static vertex buffer.
	vbh = bgfx::createVertexBuffer(
		  bgfx::makeRef(s_cubeVertices, sizeof(s_cubeVertices) )
		, PosNormalTangentTexcoordVertex::ms_decl
		);

	// Create static index buffer.
	ibh = bgfx::createIndexBuffer(bgfx::makeRef(s_cubeIndices, sizeof(s_cubeIndices) ) );

	// Create texture sampler uniforms.
	s_texColor  = bgfx::createUniform("s_texColor",  bgfx::UniformType::Int1);
	s_texNormal = bgfx::createUniform("s_texNormal", bgfx::UniformType::Int1);

	s_albedo = bgfx::createUniform("s_albedo", bgfx::UniformType::Int1);
	s_normal = bgfx::createUniform("s_normal", bgfx::UniformType::Int1);
	s_depth  = bgfx::createUniform("s_depth",  bgfx::UniformType::Int1);
	s_light  = bgfx::createUniform("s_light",  bgfx::UniformType::Int1);

	u_mtx            = bgfx::createUniform("u_mtx",            bgfx::UniformType::Mat4);
	u_lightPosRadius = bgfx::createUniform("u_lightPosRadius", bgfx::UniformType::Vec4);
	u_lightRgbInnerR = bgfx::createUniform("u_lightRgbInnerR", bgfx::UniformType::Vec4);

	// Create program from shaders.
	geomProgram    = loadProgram("vs_deferred_geom",       "fs_deferred_geom");
	lightProgram   = loadProgram("vs_deferred_light",      "fs_deferred_light");
	combineProgram = loadProgram("vs_deferred_combine",    "fs_deferred_combine");
	debugProgram   = loadProgram("vs_deferred_debug",      "fs_deferred_debug");
	lineProgram    = loadProgram("vs_deferred_debug_line", "fs_deferred_debug_line");

	// Load diffuse texture.
	textureColor  = loadTexture("fieldstone-rgba.dds");

	// Load normal texture.
	textureNormal = loadTexture("fieldstone-n.dds");

	memset(gbufferTex, bgfx::invalidHandle, sizeof(gbufferTex));
	gbuffer.idx = bgfx::invalidHandle;
	lightBuffer.idx = bgfx::invalidHandle;

	return ALL_OK;
}
void Renderer::Shutdown()
{
	if (bgfx::isValid(gbuffer) )
	{
		bgfx::destroyFrameBuffer(gbuffer);
		bgfx::destroyFrameBuffer(lightBuffer);
	}

	bgfx::destroyIndexBuffer(ibh);
	bgfx::destroyVertexBuffer(vbh);

	bgfx::destroyProgram(geomProgram);
	bgfx::destroyProgram(lightProgram);
	bgfx::destroyProgram(combineProgram);
	bgfx::destroyProgram(debugProgram);
	bgfx::destroyProgram(lineProgram);

	bgfx::destroyTexture(textureColor);
	bgfx::destroyTexture(textureNormal);
	bgfx::destroyUniform(s_texColor);
	bgfx::destroyUniform(s_texNormal);

	bgfx::destroyUniform(s_albedo);
	bgfx::destroyUniform(s_normal);
	bgfx::destroyUniform(s_depth);
	bgfx::destroyUniform(s_light);

	bgfx::destroyUniform(u_lightPosRadius);
	bgfx::destroyUniform(u_lightRgbInnerR);
	bgfx::destroyUniform(u_mtx);

	// Shutdown bgfx.
	bgfx::shutdown();
}

ERet Renderer::BeginFrame( uint32_t _width, uint32_t _height, uint32_t _reset, const float view[16], float time )
{
	if (_width  != width
	||  _height != height
	||  _reset  != reset
	||  !bgfx::isValid(gbuffer) )
	{
		// Recreate variable size render targets when resolution changes.
		width  = _width;
		height = _height;
		reset  = _reset;

		if (bgfx::isValid(gbuffer) )
		{
			bgfx::destroyFrameBuffer(gbuffer);
		}

		const uint32_t samplerFlags = 0
			| BGFX_TEXTURE_RT
			| BGFX_TEXTURE_MIN_POINT
			| BGFX_TEXTURE_MAG_POINT
			| BGFX_TEXTURE_MIP_POINT
			| BGFX_TEXTURE_U_CLAMP
			| BGFX_TEXTURE_V_CLAMP
			;
		gbufferTex[0] = bgfx::createTexture2D(width, height, 1, bgfx::TextureFormat::BGRA8, samplerFlags);
		gbufferTex[1] = bgfx::createTexture2D(width, height, 1, bgfx::TextureFormat::BGRA8, samplerFlags);
		gbufferTex[2] = bgfx::createTexture2D(width, height, 1, bgfx::TextureFormat::D24,   samplerFlags);
		gbuffer = bgfx::createFrameBuffer(BX_COUNTOF(gbufferTex), gbufferTex, true);

		if (bgfx::isValid(lightBuffer) )
		{
			bgfx::destroyFrameBuffer(lightBuffer);
		}

		lightBuffer = bgfx::createFrameBuffer(width, height, bgfx::TextureFormat::BGRA8, samplerFlags);
	}

	// Setup views
	float vp[16];
	float invMvp[16];
	{
		bgfx::setViewRect(RENDER_PASS_GEOMETRY_ID,      0, 0, width, height);
		bgfx::setViewRect(RENDER_PASS_LIGHT_ID,         0, 0, width, height);
		bgfx::setViewRect(RENDER_PASS_COMBINE_ID,       0, 0, width, height);
		bgfx::setViewRect(RENDER_PASS_DEBUG_LIGHTS_ID,  0, 0, width, height);
		bgfx::setViewRect(RENDER_PASS_DEBUG_GBUFFER_ID, 0, 0, width, height);

		bgfx::setViewFrameBuffer(RENDER_PASS_LIGHT_ID, lightBuffer);

		float proj[16];
		mtxProj(proj, 60.0f, float(width)/float(height), 0.1f, 100.0f);

		bgfx::setViewFrameBuffer(RENDER_PASS_GEOMETRY_ID, gbuffer);
		bgfx::setViewTransform(RENDER_PASS_GEOMETRY_ID, view, proj);

		bx::mtxMul(vp, view, proj);
		bx::mtxInverse(invMvp, vp);

		bx::mtxOrtho(proj, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 100.0f);
		bgfx::setViewTransform(RENDER_PASS_LIGHT_ID,   NULL, proj);
		bgfx::setViewTransform(RENDER_PASS_COMBINE_ID, NULL, proj);

		const float aspectRatio = float(height)/float(width);
		const float size = 10.0f;
		bx::mtxOrtho(proj, -size, size, size*aspectRatio, -size*aspectRatio, 0.0f, 1000.0f);
		bgfx::setViewTransform(RENDER_PASS_DEBUG_GBUFFER_ID, NULL, proj);

		bx::mtxOrtho(proj, 0.0f, (float)width, 0.0f, (float)height, 0.0f, 1000.0f);
		bgfx::setViewTransform(RENDER_PASS_DEBUG_LIGHTS_ID, NULL, proj);
	}

	const uint32_t dim = 11;
	const float offset = (float(dim-1) * 3.0f) * 0.5f;

	// Draw into geometry pass.
	for (uint32_t yy = 0; yy < dim; ++yy)
	{
		for (uint32_t xx = 0; xx < dim; ++xx)
		{
			float mtx[16];
			if (animateMesh)
			{
				bx::mtxRotateXY(mtx, time*1.023f + xx*0.21f, time*0.03f + yy*0.37f);
			}
			else
			{
				bx::mtxIdentity(mtx);
			}
			mtx[12] = -offset + float(xx)*3.0f;
			mtx[13] = -offset + float(yy)*3.0f;
			mtx[14] = 0.0f;

			// Set transform for draw call.
			bgfx::setTransform(mtx);

			// Set vertex and fragment shaders.
			bgfx::setProgram(geomProgram);

			// Set vertex and index buffer.
			bgfx::setVertexBuffer(vbh);
			bgfx::setIndexBuffer(ibh);

			// Bind textures.
			bgfx::setTexture(0, s_texColor,  textureColor);
			bgfx::setTexture(1, s_texNormal, textureNormal);

			// Set render states.
			bgfx::setState(0
				| BGFX_STATE_RGB_WRITE
				| BGFX_STATE_ALPHA_WRITE
				| BGFX_STATE_DEPTH_WRITE
				| BGFX_STATE_DEPTH_TEST_LESS
				| BGFX_STATE_MSAA
				);

			// Submit primitive for rendering to view 0.
			bgfx::submit(RENDER_PASS_GEOMETRY_ID);
		}
	}

	// Draw lights into light buffer.
	for (int32_t light = 0; light < numLights; ++light)
	{
		Sphere lightPosRadius;

		float lightTime = time * lightAnimationSpeed * (sinf(light/float(numLights) * bx::piHalf ) * 0.5f + 0.5f);
		lightPosRadius.m_center[0] = sinf( ( (lightTime + light*0.47f) + bx::piHalf*1.37f ) )*offset;
		lightPosRadius.m_center[1] = cosf( ( (lightTime + light*0.69f) + bx::piHalf*1.49f ) )*offset;
		lightPosRadius.m_center[2] = sinf( ( (lightTime + light*0.37f) + bx::piHalf*1.57f ) )*2.0f;
		lightPosRadius.m_radius = 2.0f;

		Aabb aabb;
		sphereToAabb(aabb, lightPosRadius);

		float box[8][3] =
		{
			{ aabb.m_min[0], aabb.m_min[1], aabb.m_min[2] },
			{ aabb.m_min[0], aabb.m_min[1], aabb.m_max[2] },
			{ aabb.m_min[0], aabb.m_max[1], aabb.m_min[2] },
			{ aabb.m_min[0], aabb.m_max[1], aabb.m_max[2] },
			{ aabb.m_max[0], aabb.m_min[1], aabb.m_min[2] },
			{ aabb.m_max[0], aabb.m_min[1], aabb.m_max[2] },
			{ aabb.m_max[0], aabb.m_max[1], aabb.m_min[2] },
			{ aabb.m_max[0], aabb.m_max[1], aabb.m_max[2] },
		};

		float xyz[3];
		bx::vec3MulMtxH(xyz, box[0], vp);
		float minx = xyz[0];
		float miny = xyz[1];
		float maxx = xyz[0];
		float maxy = xyz[1];
		float maxz = xyz[2];

		for (uint32_t ii = 1; ii < 8; ++ii)
		{
			bx::vec3MulMtxH(xyz, box[ii], vp);
			minx = bx::fmin(minx, xyz[0]);
			miny = bx::fmin(miny, xyz[1]);
			maxx = bx::fmax(maxx, xyz[0]);
			maxy = bx::fmax(maxy, xyz[1]);
			maxz = bx::fmax(maxz, xyz[2]);
		}

		// Cull light if it's fully behind camera.
		if (maxz >= 0.0f)
		{
			float x0 = bx::fclamp( (minx * 0.5f + 0.5f) * width,  0.0f, (float)width);
			float y0 = bx::fclamp( (miny * 0.5f + 0.5f) * height, 0.0f, (float)height);
			float x1 = bx::fclamp( (maxx * 0.5f + 0.5f) * width,  0.0f, (float)width);
			float y1 = bx::fclamp( (maxy * 0.5f + 0.5f) * height, 0.0f, (float)height);

			if (showScissorRects)
			{
				bgfx::TransientVertexBuffer tvb;
				bgfx::TransientIndexBuffer tib;
				if (bgfx::allocTransientBuffers(&tvb, DebugVertex::ms_decl, 4, &tib, 8) )
				{
					uint32_t abgr = 0x8000ff00;

					DebugVertex* vertex = (DebugVertex*)tvb.data;
					vertex->m_x = x0;
					vertex->m_y = y0;
					vertex->m_z = 0.0f;
					vertex->m_abgr = abgr;
					++vertex;

					vertex->m_x = x1;
					vertex->m_y = y0;
					vertex->m_z = 0.0f;
					vertex->m_abgr = abgr;
					++vertex;

					vertex->m_x = x1;
					vertex->m_y = y1;
					vertex->m_z = 0.0f;
					vertex->m_abgr = abgr;
					++vertex;

					vertex->m_x = x0;
					vertex->m_y = y1;
					vertex->m_z = 0.0f;
					vertex->m_abgr = abgr;

					uint16_t* indices = (uint16_t*)tib.data;
					*indices++ = 0;
					*indices++ = 1;
					*indices++ = 1;
					*indices++ = 2;
					*indices++ = 2;
					*indices++ = 3;
					*indices++ = 3;
					*indices++ = 0;

					bgfx::setProgram(lineProgram);
					bgfx::setVertexBuffer(&tvb);
					bgfx::setIndexBuffer(&tib);
					bgfx::setState(0
						| BGFX_STATE_RGB_WRITE
						| BGFX_STATE_PT_LINES
						| BGFX_STATE_BLEND_ALPHA
						);
					bgfx::submit(RENDER_PASS_DEBUG_LIGHTS_ID);
				}
			}

			uint8_t val = light&7;
			float lightRgbInnerR[4] =
			{
				val & 0x1 ? 1.0f : 0.25f,
				val & 0x2 ? 1.0f : 0.25f,
				val & 0x4 ? 1.0f : 0.25f,
				0.8f,
			};

			// Draw light.
			bgfx::setUniform(u_lightPosRadius, &lightPosRadius);
			bgfx::setUniform(u_lightRgbInnerR, lightRgbInnerR);
			bgfx::setUniform(u_mtx, invMvp);
			const uint16_t scissorHeight = uint16_t(y1-y0);
			bgfx::setScissor(uint16_t(x0), height-scissorHeight-uint16_t(y0), uint16_t(x1-x0), scissorHeight);
			bgfx::setTexture(0, s_normal, gbuffer, 1);
			bgfx::setTexture(1, s_depth,  gbuffer, 2);
			bgfx::setProgram(lightProgram);
			bgfx::setState(0
				| BGFX_STATE_RGB_WRITE
				| BGFX_STATE_ALPHA_WRITE
				| BGFX_STATE_BLEND_ADD
				);
			screenSpaceQuad( (float)width, (float)height, g_texelHalf, g_originBottomLeft);
			bgfx::submit(RENDER_PASS_LIGHT_ID);
		}
	}

	// Combine color and light buffers.
	bgfx::setTexture(0, s_albedo, gbuffer,     0);
	bgfx::setTexture(1, s_light,  lightBuffer, 0);
	bgfx::setProgram(combineProgram);
	bgfx::setState(0
		| BGFX_STATE_RGB_WRITE
		| BGFX_STATE_ALPHA_WRITE
		);
	screenSpaceQuad( (float)width, (float)height, g_texelHalf, g_originBottomLeft);
	bgfx::submit(RENDER_PASS_COMBINE_ID);

	if (showGBuffer)
	{
		const float aspectRatio = float(width)/float(height);

		// Draw debug GBuffer.
		for (uint32_t ii = 0; ii < BX_COUNTOF(gbufferTex); ++ii)
		{
			float mtx[16];
			bx::mtxSRT(mtx
				, aspectRatio, 1.0f, 1.0f
				, 0.0f, 0.0f, 0.0f
				, -7.9f - BX_COUNTOF(gbufferTex)*0.1f*0.5f + ii*2.1f*aspectRatio, 4.0f, 0.0f
				);

			bgfx::setTransform(mtx);
			bgfx::setProgram(debugProgram);
			bgfx::setVertexBuffer(vbh);
			bgfx::setIndexBuffer(ibh, 0, 6);
			bgfx::setTexture(0, s_texColor, gbufferTex[ii]);
			bgfx::setState(BGFX_STATE_RGB_WRITE);
			bgfx::submit(RENDER_PASS_DEBUG_GBUFFER_ID);
		}
	}
	return ALL_OK;
}

ERet Renderer::EndFrame()
{
	// Advance to next frame. Rendering thread will be kicked to
	// process submitted rendering primitives.
	bgfx::frame();

	return ALL_OK;
}


bool animateMesh = true;
bool showScissorRects = false;
bool showGBuffer = true;
int32_t numLights = 512;
float lightAnimationSpeed = 0.3f;