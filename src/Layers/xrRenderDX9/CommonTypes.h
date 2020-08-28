#pragma once

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

using D3D_QUERY = D3DQUERYTYPE;
constexpr auto D3D_QUERY_EVENT = D3DQUERYTYPE_EVENT;
constexpr auto D3D_QUERY_OCCLUSION = D3DQUERYTYPE_OCCLUSION;

#define DX10_ONLY(expr) do {} while (0)

using unused_t = int[0];

using VertexBufferHandle = ID3DVertexBuffer*;
using IndexBufferHandle = ID3DIndexBuffer*;
using ConstantBufferHandle = unused_t;
using HostBufferHandle = void*;

using VertexElement = D3DVERTEXELEMENT9;
using InputElementDesc = unused_t;
