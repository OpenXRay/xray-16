#pragma once

#ifdef USE_OGL

namespace glStateUtils
{
GLenum ConvertFillMode(u32 Mode);
GLenum ConvertCullMode(u32 Mode);
GLenum ConvertCmpFunction(u32 Func);
GLenum ConvertStencilOp(u32 Op);
GLenum ConvertBlendArg(u32 Arg);
GLenum ConvertBlendOp(u32 Op);
GLint ConvertTextureAddressMode(u32 Mode);
GLint ConvertTextureFilter(u32 dxFilter, GLint glFilter = (GLint)GL_NEAREST, bool MipMap = false);
};

#endif // USE_OGL
