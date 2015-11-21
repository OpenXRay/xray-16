#pragma once

#ifdef USE_OGL

namespace glStateUtils
{
	GLenum	ConvertCullMode(u32 Mode);
	GLenum	ConvertCmpFunction(u32 Func);
	GLenum	ConvertStencilOp(u32 Op);
	GLenum	ConvertBlendArg(u32 Arg);
	GLenum	ConvertBlendOp(u32 Op);
	GLenum	ConvertTextureAddressMode(u32 Mode);
	GLenum  ConvertTextureFilter(u32 dxFilter, u32 glFilter = GL_NEAREST, bool MipMap = false);
};

#endif // USE_OGL
