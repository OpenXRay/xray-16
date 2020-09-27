#include "stdafx.h"

#include "IDirect3DDevice9.h"
#include "IDirect3DQuery9.h"
#include "IDirect3DSurface9.h"
#include "IDirect3DIndexBuffer9.h"
#include "IDirect3DVertexBuffer9.h"
#include "IDirect3DTexture9.h"
#include "IDirect3DVertexDeclaration9.h"
#include "IDirect3DVertexShader9.h"
#include "IDirect3DPixelShader9.h"
#include "IDirect3DStateBlock9.h"
#include "IDirect3DCubeTexture9.h"

#include "xrD3D9-Null_OutProc.h"

const GUID DECLSPEC_SELECTANY IID_IDirect3DDevice9;

xrIDirect3DDevice9::xrIDirect3DDevice9(IDirect3D9* pDirect3D9, D3DPRESENT_PARAMETERS* pPresentationParameters)
    : m_refCount(0)
{
    APIDEBUG("xrIDirect3DDevice9::xrIDirect3DDevice9");

    m_pIDirect3D9 = pDirect3D9;
    //#ifdef D3D_DEBUG_INFO
    //-------------------------------------------------------
    memset(&CreationParameters, 0, sizeof(CreationParameters));
    memcpy(&PresentParameters, pPresentationParameters, sizeof(PresentParameters));
    memset(&DisplayMode, 0, sizeof(DisplayMode));
    memset(&Caps, 0, sizeof(Caps));

    AvailableTextureMem = 1 << 31;
    SwapChains = 0;
    Textures = 0;
    VertexBuffers = 0;
    IndexBuffers = 0;
    VertexShaders = 0;
    PixelShaders = 0;

    memset(&Viewport, 0, sizeof(Viewport));
    memset(&ProjectionMatrix, 0, sizeof(ProjectionMatrix));
    memset(&ViewMatrix, 0, sizeof(ViewMatrix));
    memset(&WorldMatrix, 0, sizeof(WorldMatrix));
    memset(&TextureMatrices, 0, sizeof(TextureMatrices));

    FVF = 0;
    VertexSize = 0;
    VertexShaderVersion = 0;
    PixelShaderVersion = 0;
    SoftwareVertexProcessing = 0;

    memset(&Material, 0, sizeof(Material));
    memset(&Lights, 0, sizeof(Lights));
    memset(&LightsEnabled, 0, sizeof(LightsEnabled));

    memset(&GammaRamp, 0, sizeof(GammaRamp));
    memset(&ScissorRect, 0, sizeof(ScissorRect));
    DialogBoxMode = FALSE;
    //#endif
};

ULONG xrIDirect3DDevice9::AddRef(void)
{
    APIDEBUG("xrIDirect3DDevice9::AddRef");
    m_refCount++;
    return m_refCount;
}

ULONG xrIDirect3DDevice9::Release(void)
{
    APIDEBUG("xrIDirect3DDevice9::Release");
    m_refCount--;
    if (m_refCount < 0)
    {
        delete this;
        return 0;
    }
    return m_refCount;
}

HRESULT xrIDirect3DDevice9::QueryInterface(const IID& iid, void FAR* FAR* ppvObj)
{
    APIDEBUG("xrIDirect3DDevice9::QueryInterface");
    if (iid == IID_IUnknown || iid == IID_IDirect3DDevice9)
    {
        *ppvObj = this;
        AddRef();
        return NOERROR;
    }
    return E_NOINTERFACE;
}

HRESULT xrIDirect3DDevice9::TestCooperativeLevel()
{
    APIDEBUG("xrIDirect3DDevice9::TestCooperativeLevel");
    return S_OK;
};
UINT xrIDirect3DDevice9::GetAvailableTextureMem()
{
    APIDEBUG("xrIDirect3DDevice9::GetAvailableTextureMem");
    return AvailableTextureMem;
};
HRESULT xrIDirect3DDevice9::EvictManagedResources()
{
    APIDEBUG("xrIDirect3DDevice9::EvictManagedResources");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetDirect3D(IDirect3D9** ppD3D9)
{
    APIDEBUG("xrIDirect3DDevice9::GetDirect3D");

    m_pIDirect3D9->AddRef();
    *ppD3D9 = m_pIDirect3D9;
    return S_OK;
};

HRESULT xrIDirect3DDevice9::GetDisplayMode(UINT iSwapChain, D3DDISPLAYMODE* pMode)
{
    APIDEBUG("xrIDirect3DDevice9::GetDisplayMode");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS* pParameters)
{
    APIDEBUG("xrIDirect3DDevice9::GetCreationParameters");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::SetCursorProperties(UINT XHotSpot, UINT YHotSpot, IDirect3DSurface9* pCursorBitmap)
{
    APIDEBUG("xrIDirect3DDevice9::SetCursorProperties");
    return S_OK;
};
void xrIDirect3DDevice9::SetCursorPosition(int X, int Y, DWORD Flags)
{
    APIDEBUG("xrIDirect3DDevice9::SetCursorPosition");
};
BOOL xrIDirect3DDevice9::ShowCursor(BOOL bShow)
{
    APIDEBUG("xrIDirect3DDevice9::ShowCursor");
    return bShow;
};
HRESULT xrIDirect3DDevice9::CreateAdditionalSwapChain(
    D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain9** pSwapChain)
{
    APIDEBUG("xrIDirect3DDevice9::CreateAdditionalSwapChain");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetSwapChain(UINT iSwapChain, IDirect3DSwapChain9** pSwapChain)
{
    APIDEBUG("xrIDirect3DDevice9::GetSwapChain");
    return S_OK;
};
UINT xrIDirect3DDevice9::GetNumberOfSwapChains()
{
    APIDEBUG("xrIDirect3DDevice9::GetNumberOfSwapChains");
    return 1;
};
HRESULT xrIDirect3DDevice9::Reset(D3DPRESENT_PARAMETERS* pPresentationParameters)
{
    APIDEBUG("xrIDirect3DDevice9::Reset");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::Present(
    CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion)
{
    APIDEBUG("xrIDirect3DDevice9::Present");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetBackBuffer(
    UINT iSwapChain, UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9** ppBackBuffer)
{
    APIDEBUG("xrIDirect3DDevice9::GetBackBuffer");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetRasterStatus(UINT iSwapChain, D3DRASTER_STATUS* pRasterStatus)
{
    APIDEBUG("xrIDirect3DDevice9::GetRasterStatus");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::SetDialogBoxMode(BOOL bEnableDialogs)
{
    APIDEBUG("xrIDirect3DDevice9::SetDialogBoxMode");
    return S_OK;
};
void xrIDirect3DDevice9::SetGammaRamp(UINT iSwapChain, DWORD Flags, CONST D3DGAMMARAMP* pRamp)
{
    APIDEBUG("xrIDirect3DDevice9::SetGammaRamp");
};
void xrIDirect3DDevice9::GetGammaRamp(UINT iSwapChain, D3DGAMMARAMP* pRamp)
{
    APIDEBUG("xrIDirect3DDevice9::GetGammaRamp");
};

HRESULT xrIDirect3DDevice9::CreateTexture(UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format,
    D3DPOOL Pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle)
{
    APIDEBUG("xrIDirect3DDevice9::CreateTexture");

    *ppTexture = NULL;
    xrIDirect3DTexture9* I = new xrIDirect3DTexture9(this, Width, Height, Levels, Usage, Format, Pool);
    *ppTexture = I;

    return S_OK;
};

HRESULT xrIDirect3DDevice9::CreateVolumeTexture(UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage,
    D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture9** ppVolumeTexture, HANDLE* pSharedHandle)
{
    APIDEBUG("xrIDirect3DDevice9::CreateVolumeTexture");
    return S_OK;
};

HRESULT xrIDirect3DDevice9::CreateCubeTexture(UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool,
    IDirect3DCubeTexture9** ppCubeTexture, HANDLE* pSharedHandle)
{
    APIDEBUG("xrIDirect3DDevice9::CreateCubeTexture");

    *ppCubeTexture = NULL;
    xrIDirect3DCubeTexture9* I = new xrIDirect3DCubeTexture9(this, EdgeLength, EdgeLength, Levels, Usage, Format, Pool);
    *ppCubeTexture = I;

    return S_OK;
};

HRESULT xrIDirect3DDevice9::CreateVertexBuffer(
    UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer9** ppVertexBuffer, HANDLE* pSharedHandle)
{
    APIDEBUG("xrIDirect3DDevice9::CreateVertexBuffer");

    *ppVertexBuffer = NULL;
    xrIDirect3DVertexBuffer9* I = new xrIDirect3DVertexBuffer9(this, Length, Usage, FVF, Pool);
    *ppVertexBuffer = I;

    return S_OK;
};
HRESULT xrIDirect3DDevice9::CreateIndexBuffer(UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool,
    IDirect3DIndexBuffer9** ppIndexBuffer, HANDLE* pSharedHandle)
{
    APIDEBUG("xrIDirect3DDevice9::CreateIndexBuffer");

    *ppIndexBuffer = NULL;
    xrIDirect3DIndexBuffer9* I = new xrIDirect3DIndexBuffer9(this, Length, Usage, Format, Pool);
    *ppIndexBuffer = I;

    return S_OK;
};
HRESULT xrIDirect3DDevice9::CreateRenderTarget(UINT Width, UINT Height, D3DFORMAT Format,
    D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Lockable, IDirect3DSurface9** ppSurface,
    HANDLE* pSharedHandle)
{
    APIDEBUG("xrIDirect3DDevice9::CreateRenderTarget");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::CreateDepthStencilSurface(UINT Width, UINT Height, D3DFORMAT Format,
    D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Discard, IDirect3DSurface9** ppSurface,
    HANDLE* pSharedHandle)
{
    APIDEBUG("xrIDirect3DDevice9::CreateDepthStencilSurface");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::UpdateSurface(IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect,
    IDirect3DSurface9* pDestinationSurface, CONST POINT* pDestPoint)
{
    APIDEBUG("xrIDirect3DDevice9::UpdateSurface");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::UpdateTexture(
    IDirect3DBaseTexture9* pSourceTexture, IDirect3DBaseTexture9* pDestinationTexture)
{
    APIDEBUG("xrIDirect3DDevice9::UpdateTexture");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetRenderTargetData(IDirect3DSurface9* pRenderTarget, IDirect3DSurface9* pDestSurface)
{
    APIDEBUG("xrIDirect3DDevice9::GetRenderTargetData");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetFrontBufferData(UINT iSwapChain, IDirect3DSurface9* pDestSurface)
{
    APIDEBUG("xrIDirect3DDevice9::GetFrontBufferData");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::StretchRect(IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect,
    IDirect3DSurface9* pDestSurface, CONST RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter)
{
    APIDEBUG("xrIDirect3DDevice9::StretchRect");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::ColorFill(IDirect3DSurface9* pSurface, CONST RECT* pRect, D3DCOLOR color)
{
    APIDEBUG("xrIDirect3DDevice9::ColorFill");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::CreateOffscreenPlainSurface(
    UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle)
{
    APIDEBUG("xrIDirect3DDevice9::CreateOffscreenPlainSurface");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::SetRenderTarget(DWORD RenderTargetIndex, IDirect3DSurface9* pRenderTarget)
{
    APIDEBUG("xrIDirect3DDevice9::SetRenderTarget");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetRenderTarget(DWORD RenderTargetIndex, IDirect3DSurface9** ppRenderTarget)
{
    APIDEBUG("xrIDirect3DDevice9::GetRenderTarget");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::SetDepthStencilSurface(IDirect3DSurface9* pNewZStencil)
{
    APIDEBUG("xrIDirect3DDevice9::SetDepthStencilSurface");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetDepthStencilSurface(IDirect3DSurface9** ppZStencilSurface)
{
    APIDEBUG("xrIDirect3DDevice9::GetDepthStencilSurface");

    *ppZStencilSurface = NULL;
    xrIDirect3DSurface9* I = new xrIDirect3DSurface9(this, 0, 0, D3DFORMAT(0), D3DMULTISAMPLE_TYPE(0), 0);
    *ppZStencilSurface = I;

    return S_OK;
};
HRESULT xrIDirect3DDevice9::BeginScene()
{
    APIDEBUG("xrIDirect3DDevice9::BeginScene");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::EndScene()
{
    APIDEBUG("xrIDirect3DDevice9::EndScene");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::Clear(
    DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil)
{
    APIDEBUG("xrIDirect3DDevice9::Clear");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::SetTransform(D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix)
{
    APIDEBUG("xrIDirect3DDevice9::SetTransform");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetTransform(D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix)
{
    APIDEBUG("xrIDirect3DDevice9::GetTransform");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::MultiplyTransform(D3DTRANSFORMSTATETYPE, CONST D3DMATRIX*)
{
    APIDEBUG("xrIDirect3DDevice9::MultiplyTransform");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::SetViewport(CONST D3DVIEWPORT9* pViewport)
{
    APIDEBUG("xrIDirect3DDevice9::SetViewport");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetViewport(D3DVIEWPORT9* pViewport)
{
    APIDEBUG("xrIDirect3DDevice9::GetViewport");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::SetMaterial(CONST D3DMATERIAL9* pMaterial)
{
    APIDEBUG("xrIDirect3DDevice9::SetMaterial");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetMaterial(D3DMATERIAL9* pMaterial)
{
    APIDEBUG("xrIDirect3DDevice9::GetMaterial");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::SetLight(DWORD Index, CONST D3DLIGHT9*)
{
    APIDEBUG("xrIDirect3DDevice9::SetLight");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetLight(DWORD Index, D3DLIGHT9*)
{
    APIDEBUG("xrIDirect3DDevice9::GetLight");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::LightEnable(DWORD Index, BOOL Enable)
{
    APIDEBUG("xrIDirect3DDevice9::LightEnable");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetLightEnable(DWORD Index, BOOL* pEnable)
{
    APIDEBUG("xrIDirect3DDevice9::GetLightEnable");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::SetClipPlane(DWORD Index, CONST float* pPlane)
{
    APIDEBUG("xrIDirect3DDevice9::SetClipPlane");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetClipPlane(DWORD Index, float* pPlane)
{
    APIDEBUG("xrIDirect3DDevice9::GetClipPlane");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::SetRenderState(D3DRENDERSTATETYPE State, DWORD Value)
{
    APIDEBUG("xrIDirect3DDevice9::SetRenderState");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetRenderState(D3DRENDERSTATETYPE State, DWORD* pValue)
{
    APIDEBUG("xrIDirect3DDevice9::GetRenderState");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::CreateStateBlock(D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9** ppSB)
{
    APIDEBUG("xrIDirect3DDevice9::CreateStateBlock");
    *ppSB = NULL;
    xrIDirect3DStateBlock9* I = new xrIDirect3DStateBlock9(this);
    *ppSB = I;
    return S_OK;
};
HRESULT xrIDirect3DDevice9::BeginStateBlock()
{
    APIDEBUG("xrIDirect3DDevice9::BeginStateBlock");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::EndStateBlock(IDirect3DStateBlock9** ppSB)
{
    APIDEBUG("xrIDirect3DDevice9::EndStateBlock");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::SetClipStatus(CONST D3DCLIPSTATUS9* pClipStatus)
{
    APIDEBUG("xrIDirect3DDevice9::SetClipStatus");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetClipStatus(D3DCLIPSTATUS9* pClipStatus)
{
    APIDEBUG("xrIDirect3DDevice9::GetClipStatus");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetTexture(DWORD Stage, IDirect3DBaseTexture9** ppTexture)
{
    APIDEBUG("xrIDirect3DDevice9::GetTexture");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::SetTexture(DWORD Stage, IDirect3DBaseTexture9* pTexture)
{
    APIDEBUG("xrIDirect3DDevice9::SetTexture");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue)
{
    APIDEBUG("xrIDirect3DDevice9::GetTextureStageState");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value)
{
    APIDEBUG("xrIDirect3DDevice9::SetTextureStageState");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD* pValue)
{
    APIDEBUG("xrIDirect3DDevice9::GetSamplerState");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::SetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value)
{
    APIDEBUG("xrIDirect3DDevice9::SetSamplerState");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::ValidateDevice(DWORD* pNumPasses)
{
    APIDEBUG("xrIDirect3DDevice9::ValidateDevice");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::SetPaletteEntries(UINT PaletteNumber, CONST PALETTEENTRY* pEntries)
{
    APIDEBUG("xrIDirect3DDevice9::SetPaletteEntries");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetPaletteEntries(UINT PaletteNumber, PALETTEENTRY* pEntries)
{
    APIDEBUG("xrIDirect3DDevice9::GetPaletteEntries");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::SetCurrentTexturePalette(UINT PaletteNumber)
{
    APIDEBUG("xrIDirect3DDevice9::SetCurrentTexturePalette");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetCurrentTexturePalette(UINT* PaletteNumber)
{
    APIDEBUG("xrIDirect3DDevice9::GetCurrentTexturePalette");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::SetScissorRect(CONST RECT* pRect)
{
    APIDEBUG("xrIDirect3DDevice9::SetScissorRect");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetScissorRect(RECT* pRect)
{
    APIDEBUG("xrIDirect3DDevice9::GetScissorRect");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::SetSoftwareVertexProcessing(BOOL bSoftware)
{
    APIDEBUG("xrIDirect3DDevice9::SetSoftwareVertexProcessing");
    return S_OK;
};
BOOL xrIDirect3DDevice9::GetSoftwareVertexProcessing()
{
    APIDEBUG("xrIDirect3DDevice9::GetSoftwareVertexProcessing");
    return TRUE;
};
HRESULT xrIDirect3DDevice9::SetNPatchMode(float nSegments)
{
    APIDEBUG("xrIDirect3DDevice9::SetNPatchMode");
    return S_OK;
};
float xrIDirect3DDevice9::GetNPatchMode()
{
    APIDEBUG("xrIDirect3DDevice9::GetNPatchMode");
    return 0.0f;
};
HRESULT xrIDirect3DDevice9::DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount)
{
    APIDEBUG("xrIDirect3DDevice9::DrawPrimitive");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::DrawIndexedPrimitive(
    D3DPRIMITIVETYPE, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount)
{
    APIDEBUG("xrIDirect3DDevice9::DrawIndexedPrimitive");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::DrawPrimitiveUP(
    D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
    APIDEBUG("xrIDirect3DDevice9::DrawPrimitiveUP");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex,
    UINT NumVertices, UINT PrimitiveCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat,
    CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
    APIDEBUG("xrIDirect3DDevice9::DrawIndexedPrimitiveUP");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::ProcessVertices(UINT SrcStartIndex, UINT DestIndex, UINT VertexCount,
    IDirect3DVertexBuffer9* pDestBuffer, IDirect3DVertexDeclaration9* pVertexDecl, DWORD Flags)
{
    APIDEBUG("xrIDirect3DDevice9::ProcessVertices");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::CreateVertexDeclaration(
    CONST D3DVERTEXELEMENT9* pVertexElements, IDirect3DVertexDeclaration9** ppDecl)
{
    APIDEBUG("xrIDirect3DDevice9::CreateVertexDeclaration");
    *ppDecl = NULL;
    xrIDirect3DVertexDeclaration9* I = new xrIDirect3DVertexDeclaration9(this);
    *ppDecl = I;
    return S_OK;
};
HRESULT xrIDirect3DDevice9::SetVertexDeclaration(IDirect3DVertexDeclaration9* pDecl)
{
    APIDEBUG("xrIDirect3DDevice9::SetVertexDeclaration");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetVertexDeclaration(IDirect3DVertexDeclaration9** ppDecl)
{
    APIDEBUG("xrIDirect3DDevice9::GetVertexDeclaration");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::SetFVF(DWORD FVF)
{
    APIDEBUG("xrIDirect3DDevice9::SetFVF");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetFVF(DWORD* pFVF)
{
    APIDEBUG("xrIDirect3DDevice9::GetFVF");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::CreateVertexShader(CONST DWORD* pFunction, IDirect3DVertexShader9** ppShader)
{
    APIDEBUG("xrIDirect3DDevice9::CreateVertexShader");
    *ppShader = NULL;
    xrIDirect3DVertexShader9* I = new xrIDirect3DVertexShader9(this);
    *ppShader = I;
    return S_OK;
};
HRESULT xrIDirect3DDevice9::SetVertexShader(IDirect3DVertexShader9* pShader)
{
    APIDEBUG("xrIDirect3DDevice9::SetVertexShader");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetVertexShader(IDirect3DVertexShader9** ppShader)
{
    APIDEBUG("xrIDirect3DDevice9::GetVertexShader");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::SetVertexShaderConstantF(UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount)
{
    APIDEBUG("xrIDirect3DDevice9::SetVertexShaderConstantF");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetVertexShaderConstantF(UINT StartRegister, float* pConstantData, UINT Vector4fCount)
{
    APIDEBUG("xrIDirect3DDevice9::GetVertexShaderConstantF");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::SetVertexShaderConstantI(UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount)
{
    APIDEBUG("xrIDirect3DDevice9::SetVertexShaderConstantI");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetVertexShaderConstantI(UINT StartRegister, int* pConstantData, UINT Vector4iCount)
{
    APIDEBUG("xrIDirect3DDevice9::GetVertexShaderConstantI");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::SetVertexShaderConstantB(UINT StartRegister, CONST BOOL* pConstantData, UINT BoolCount)
{
    APIDEBUG("xrIDirect3DDevice9::SetVertexShaderConstantB");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetVertexShaderConstantB(UINT StartRegister, BOOL* pConstantData, UINT BoolCount)
{
    APIDEBUG("xrIDirect3DDevice9::GetVertexShaderConstantB");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::SetStreamSource(
    UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT Stride)
{
    APIDEBUG("xrIDirect3DDevice9::SetStreamSource");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetStreamSource(
    UINT StreamNumber, IDirect3DVertexBuffer9** ppStreamData, UINT* pOffsetInBytes, UINT* pStride)
{
    APIDEBUG("xrIDirect3DDevice9::GetStreamSource");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::SetStreamSourceFreq(UINT StreamNumber, UINT Setting)
{
    APIDEBUG("xrIDirect3DDevice9::SetStreamSourceFreq");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetStreamSourceFreq(UINT StreamNumber, UINT* pSetting)
{
    APIDEBUG("xrIDirect3DDevice9::GetStreamSourceFreq");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::SetIndices(IDirect3DIndexBuffer9* pIndexData)
{
    APIDEBUG("xrIDirect3DDevice9::SetIndices");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetIndices(IDirect3DIndexBuffer9** ppIndexData)
{
    APIDEBUG("xrIDirect3DDevice9::GetIndices");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::CreatePixelShader(CONST DWORD* pFunction, IDirect3DPixelShader9** ppShader)
{
    APIDEBUG("xrIDirect3DDevice9::CreatePixelShader");
    *ppShader = NULL;
    xrIDirect3DPixelShader9* I = new xrIDirect3DPixelShader9(this);
    *ppShader = I;
    return S_OK;
};
HRESULT xrIDirect3DDevice9::SetPixelShader(IDirect3DPixelShader9* pShader)
{
    APIDEBUG("xrIDirect3DDevice9::SetPixelShader");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetPixelShader(IDirect3DPixelShader9** ppShader)
{
    APIDEBUG("xrIDirect3DDevice9::GetPixelShader");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::SetPixelShaderConstantF(UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount)
{
    APIDEBUG("xrIDirect3DDevice9::SetPixelShaderConstantF");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetPixelShaderConstantF(UINT StartRegister, float* pConstantData, UINT Vector4fCount)
{
    APIDEBUG("xrIDirect3DDevice9::GetPixelShaderConstantF");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::SetPixelShaderConstantI(UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount)
{
    APIDEBUG("xrIDirect3DDevice9::SetPixelShaderConstantI");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetPixelShaderConstantI(UINT StartRegister, int* pConstantData, UINT Vector4iCount)
{
    APIDEBUG("xrIDirect3DDevice9::GetPixelShaderConstantI");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::SetPixelShaderConstantB(UINT StartRegister, CONST BOOL* pConstantData, UINT BoolCount)
{
    APIDEBUG("xrIDirect3DDevice9::SetPixelShaderConstantB");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::GetPixelShaderConstantB(UINT StartRegister, BOOL* pConstantData, UINT BoolCount)
{
    APIDEBUG("xrIDirect3DDevice9::GetPixelShaderConstantB");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::DrawRectPatch(UINT Handle, CONST float* pNumSegs, CONST D3DRECTPATCH_INFO* pRectPatchInfo)
{
    APIDEBUG("xrIDirect3DDevice9::DrawRectPatch");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::DrawTriPatch(UINT Handle, CONST float* pNumSegs, CONST D3DTRIPATCH_INFO* pTriPatchInfo)
{
    APIDEBUG("xrIDirect3DDevice9::DrawTriPatch");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::DeletePatch(UINT Handle)
{
    APIDEBUG("xrIDirect3DDevice9::DeletePatch");
    return S_OK;
};
HRESULT xrIDirect3DDevice9::CreateQuery(D3DQUERYTYPE Type, IDirect3DQuery9** ppQuery)
{
    APIDEBUG("xrIDirect3DDevice9::CreateQuery");
    *ppQuery = NULL;
    xrIDirect3DQuery9* I = new xrIDirect3DQuery9(this, Type);
    *ppQuery = I;
    return S_OK;
};
