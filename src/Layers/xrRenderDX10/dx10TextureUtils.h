#ifndef dx10TextureUtils_included
#define dx10TextureUtils_included
#pragma once

namespace dx10TextureUtils
{
DXGI_FORMAT ConvertTextureFormat(D3DFORMAT dx9FMT);
D3DFORMAT ConvertTextureFormat(DXGI_FORMAT dx10FMT);
}

#endif //	dx10TextureUtils_included
