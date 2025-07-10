#pragma once

#ifdef USE_OGL

namespace xray::render::RENDER_NAMESPACE
{
namespace glTextureUtils
{
GLenum ConvertTextureFormat(D3DFORMAT dx9FMT);
} // namespace glTextureUtils
} // namespace xray::render::RENDER_NAMESPACE

#endif // USE_OGL
