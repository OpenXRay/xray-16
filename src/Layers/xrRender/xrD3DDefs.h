#pragma once
#ifndef xrD3DDefs_included
#define xrD3DDefs_included

#if defined(USE_OGL)

// TODO: Get rid of D3D types.
#if defined(WINDOWS)
#include <d3d9types.h>
#endif

class glState;

typedef enum D3D_CLEAR_FLAG {
	D3D_CLEAR_DEPTH = 0x1L,
	D3D_CLEAR_STENCIL = 0x2L
} D3D_CLEAR_FLAG;

typedef enum D3D_COMPARISON_FUNC { 
  D3D_COMPARISON_NEVER          = GL_NEVER,
  D3D_COMPARISON_LESS           = GL_LESS,
  D3D_COMPARISON_EQUAL          = GL_EQUAL,
  D3D_COMPARISON_LESS_EQUAL     = GL_LEQUAL,
  D3D_COMPARISON_GREATER        = GL_GREATER,
  D3D_COMPARISON_NOT_EQUAL      = GL_NOTEQUAL,
  D3D_COMPARISON_GREATER_EQUAL  = GL_GEQUAL,
  D3D_COMPARISON_ALWAYS         = GL_ALWAYS
} D3D_COMPARISON_FUNC;

#define DX10_ONLY(expr)			do {} while (0)

#elif defined(USE_DX11) || defined(USE_DX10)
#include "Layers/xrRenderDX10/DXCommonTypes.h"
#else // USE_DX10

typedef IDirect3DVertexShader9 ID3DVertexShader;
typedef IDirect3DPixelShader9 ID3DPixelShader;
typedef ID3DXBuffer ID3DBlob;
typedef D3DXMACRO D3D_SHADER_MACRO;
typedef D3DDEVTYPE D3D_DRIVER_TYPE;
typedef IDirect3DQuery9 ID3DQuery;
typedef D3DVIEWPORT9 D3D_VIEWPORT;
typedef ID3DXInclude ID3DInclude;
typedef IDirect3DTexture9 ID3DTexture2D;
typedef IDirect3DSurface9 ID3DRenderTargetView;
typedef IDirect3DSurface9 ID3DDepthStencilView;
typedef IDirect3DDevice9 ID3DDevice;
typedef IDirect3DBaseTexture9 ID3DBaseTexture;
typedef D3DSURFACE_DESC D3D_TEXTURE2D_DESC;
typedef IDirect3DVertexBuffer9 ID3DVertexBuffer;
typedef IDirect3DIndexBuffer9 ID3DIndexBuffer;
typedef IDirect3DVolumeTexture9 ID3DTexture3D;
typedef IDirect3DStateBlock9 ID3DState;

#define DX10_ONLY(expr) do {} while (0)

#endif // USE_DX10

#endif // xrD3DDefs_included
