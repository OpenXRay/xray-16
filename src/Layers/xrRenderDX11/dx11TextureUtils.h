#ifndef dx11TextureUtils_included
#define dx11TextureUtils_included
#pragma once

// hack for dx9... That's it. I don't know if there's already DS64 format.
// If true, then replace with another 4 chars
#define D3DFMT_D32S8X24 (D3DFORMAT)MAKEFOURCC('D', 'S', '6', '4')

namespace dx11TextureUtils
{
DXGI_FORMAT ConvertTextureFormat(D3DFORMAT dx9FMT);
D3DFORMAT ConvertTextureFormat(DXGI_FORMAT dx11FMT);
}

#endif //	dx11TextureUtils_included
