#include "stdafx.h"

#ifdef __cplusplus
extern "C" {
#endif

class xrIDirect3DDevice9 : public IDirect3DDevice9
{
protected:
    signed int m_refCount;
    IDirect3D9* m_pIDirect3D9;

public:
    xrIDirect3DDevice9(IDirect3D9* pDirect3D9, D3DPRESENT_PARAMETERS* pPresentationParameters);
    /*** IUnknown methods ***/
    HRESULT __stdcall QueryInterface(REFIID riid, void** ppvObj);
    ULONG __stdcall AddRef();
    ULONG __stdcall Release();

    HRESULT __stdcall TestCooperativeLevel();

    unsigned int __stdcall GetAvailableTextureMem();
    HRESULT __stdcall EvictManagedResources();
    HRESULT __stdcall GetDirect3D(IDirect3D9** ppD3D9);
    HRESULT __stdcall GetDeviceCaps(D3DCAPS9* pCaps);
    HRESULT __stdcall GetDisplayMode(unsigned int iSwapChain, D3DDISPLAYMODE* pMode);
    HRESULT __stdcall GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS* pParameters);
    HRESULT __stdcall SetCursorProperties(unsigned int XHotSpot, unsigned int YHotSpot, IDirect3DSurface9* pCursorBitmap);
    void __stdcall SetCursorPosition(int X, int Y, DWORD Flags);
    BOOL __stdcall ShowCursor(BOOL bShow);
    HRESULT __stdcall CreateAdditionalSwapChain(
        D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain9** pSwapChain);
    HRESULT __stdcall GetSwapChain(unsigned int iSwapChain, IDirect3DSwapChain9** pSwapChain);
    unsigned int __stdcall GetNumberOfSwapChains();
    HRESULT __stdcall Reset(D3DPRESENT_PARAMETERS* pPresentationParameters);
    HRESULT __stdcall Present(
        const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion);
    HRESULT __stdcall GetBackBuffer(
        unsigned int iSwapChain, unsigned int iBackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9** ppBackBuffer);
    HRESULT __stdcall GetRasterStatus(unsigned int iSwapChain, D3DRASTER_STATUS* pRasterStatus);
    HRESULT __stdcall SetDialogBoxMode(BOOL bEnableDialogs);
    void __stdcall SetGammaRamp(unsigned int iSwapChain, DWORD Flags, const D3DGAMMARAMP* pRamp);
    void __stdcall GetGammaRamp(unsigned int iSwapChain, D3DGAMMARAMP* pRamp);
    HRESULT __stdcall CreateTexture(unsigned int Width, unsigned int Height, unsigned int Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool,
        IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle);
    HRESULT __stdcall CreateVolumeTexture(unsigned int Width, unsigned int Height, unsigned int Depth, unsigned int Levels, DWORD Usage,
        D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture9** ppVolumeTexture, HANDLE* pSharedHandle);
    HRESULT __stdcall CreateCubeTexture(unsigned int EdgeLength, unsigned int Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool,
        IDirect3DCubeTexture9** ppCubeTexture, HANDLE* pSharedHandle);
    HRESULT __stdcall CreateVertexBuffer(unsigned int Length, DWORD Usage, DWORD FVF, D3DPOOL Pool,
        IDirect3DVertexBuffer9** ppVertexBuffer, HANDLE* pSharedHandle);
    HRESULT __stdcall CreateIndexBuffer(unsigned int Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool,
        IDirect3DIndexBuffer9** ppIndexBuffer, HANDLE* pSharedHandle);
    HRESULT __stdcall CreateRenderTarget(unsigned int Width, unsigned int Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample,
        DWORD MultisampleQuality, BOOL Lockable, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle);
    HRESULT __stdcall CreateDepthStencilSurface(unsigned int Width, unsigned int Height, D3DFORMAT Format,
        D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Discard, IDirect3DSurface9** ppSurface,
        HANDLE* pSharedHandle);
    HRESULT __stdcall UpdateSurface(IDirect3DSurface9* pSourceSurface, const RECT* pSourceRect,
        IDirect3DSurface9* pDestinationSurface, const POINT* pDestPoint);
    HRESULT __stdcall UpdateTexture(IDirect3DBaseTexture9* pSourceTexture, IDirect3DBaseTexture9* pDestinationTexture);
    HRESULT __stdcall GetRenderTargetData(IDirect3DSurface9* pRenderTarget, IDirect3DSurface9* pDestSurface);
    HRESULT __stdcall GetFrontBufferData(unsigned int iSwapChain, IDirect3DSurface9* pDestSurface);
    HRESULT __stdcall StretchRect(IDirect3DSurface9* pSourceSurface, const RECT* pSourceRect,
        IDirect3DSurface9* pDestSurface, const RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter);
    HRESULT __stdcall ColorFill(IDirect3DSurface9* pSurface, const RECT* pRect, D3DCOLOR color);
    HRESULT __stdcall CreateOffscreenPlainSurface(
        unsigned int Width, unsigned int Height, D3DFORMAT Format, D3DPOOL Pool, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle);
    HRESULT __stdcall SetRenderTarget(DWORD RenderTargetIndex, IDirect3DSurface9* pRenderTarget);
    HRESULT __stdcall GetRenderTarget(DWORD RenderTargetIndex, IDirect3DSurface9** ppRenderTarget);
    HRESULT __stdcall SetDepthStencilSurface(IDirect3DSurface9* pNewZStencil);
    HRESULT __stdcall GetDepthStencilSurface(IDirect3DSurface9** ppZStencilSurface);
    HRESULT __stdcall BeginScene();
    HRESULT __stdcall EndScene();
    HRESULT __stdcall Clear(DWORD Count, const D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil);
    HRESULT __stdcall SetTransform(D3DTRANSFORMSTATETYPE State, const D3DMATRIX* pMatrix);
    HRESULT __stdcall GetTransform(D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix);
    HRESULT __stdcall MultiplyTransform(D3DTRANSFORMSTATETYPE, const D3DMATRIX*);
    HRESULT __stdcall SetViewport(const D3DVIEWPORT9* pViewport);
    HRESULT __stdcall GetViewport(D3DVIEWPORT9* pViewport);
    HRESULT __stdcall SetMaterial(const D3DMATERIAL9* pMaterial);
    HRESULT __stdcall GetMaterial(D3DMATERIAL9* pMaterial);
    HRESULT __stdcall SetLight(DWORD Index, const D3DLIGHT9*);
    HRESULT __stdcall GetLight(DWORD Index, D3DLIGHT9*);
    HRESULT __stdcall LightEnable(DWORD Index, BOOL Enable);
    HRESULT __stdcall GetLightEnable(DWORD Index, BOOL* pEnable);
    HRESULT __stdcall SetClipPlane(DWORD Index, const float* pPlane);
    HRESULT __stdcall GetClipPlane(DWORD Index, float* pPlane);
    HRESULT __stdcall SetRenderState(D3DRENDERSTATETYPE State, DWORD Value);
    HRESULT __stdcall GetRenderState(D3DRENDERSTATETYPE State, DWORD* pValue);
    HRESULT __stdcall CreateStateBlock(D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9** ppSB);
    HRESULT __stdcall BeginStateBlock();
    HRESULT __stdcall EndStateBlock(IDirect3DStateBlock9** ppSB);
    HRESULT __stdcall SetClipStatus(const D3DCLIPSTATUS9* pClipStatus);
    HRESULT __stdcall GetClipStatus(D3DCLIPSTATUS9* pClipStatus);
    HRESULT __stdcall GetTexture(DWORD Stage, IDirect3DBaseTexture9** ppTexture);
    HRESULT __stdcall SetTexture(DWORD Stage, IDirect3DBaseTexture9* pTexture);
    HRESULT __stdcall GetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue);
    HRESULT __stdcall SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value);
    HRESULT __stdcall GetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD* pValue);
    HRESULT __stdcall SetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value);
    HRESULT __stdcall ValidateDevice(DWORD* pNumPasses);
    HRESULT __stdcall SetPaletteEntries(unsigned int PaletteNumber, const PALETTEENTRY* pEntries);
    HRESULT __stdcall GetPaletteEntries(unsigned int PaletteNumber, PALETTEENTRY* pEntries);
    HRESULT __stdcall SetCurrentTexturePalette(unsigned int PaletteNumber);
    HRESULT __stdcall GetCurrentTexturePalette(unsigned int* PaletteNumber);
    HRESULT __stdcall SetScissorRect(const RECT* pRect);
    HRESULT __stdcall GetScissorRect(RECT* pRect);
    HRESULT __stdcall SetSoftwareVertexProcessing(BOOL bSoftware);
    BOOL __stdcall GetSoftwareVertexProcessing();
    HRESULT __stdcall SetNPatchMode(float nSegments);
    float __stdcall GetNPatchMode();
    HRESULT __stdcall DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, unsigned int StartVertex, unsigned int PrimitiveCount);
    HRESULT __stdcall DrawIndexedPrimitive(
        D3DPRIMITIVETYPE, INT BaseVertexIndex, unsigned int MinVertexIndex, unsigned int NumVertices, unsigned int startIndex, unsigned int primCount);
    HRESULT __stdcall DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, unsigned int PrimitiveCount,
        const void* pVertexStreamZeroData, unsigned int VertexStreamZeroStride);
    HRESULT __stdcall DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, unsigned int MinVertexIndex, unsigned int NumVertices,
        unsigned int PrimitiveCount, const void* pIndexData, D3DFORMAT IndexDataFormat, const void* pVertexStreamZeroData,
        unsigned int VertexStreamZeroStride);
    HRESULT __stdcall ProcessVertices(unsigned int SrcStartIndex, unsigned int DestIndex, unsigned int VertexCount,
        IDirect3DVertexBuffer9* pDestBuffer, IDirect3DVertexDeclaration9* pVertexDecl, DWORD Flags);
    HRESULT __stdcall CreateVertexDeclaration(
        const D3DVERTEXELEMENT9* pVertexElements, IDirect3DVertexDeclaration9** ppDecl);
    HRESULT __stdcall SetVertexDeclaration(IDirect3DVertexDeclaration9* pDecl);
    HRESULT __stdcall GetVertexDeclaration(IDirect3DVertexDeclaration9** ppDecl);
    HRESULT __stdcall SetFVF(DWORD FVF);
    HRESULT __stdcall GetFVF(DWORD* pFVF);
    HRESULT __stdcall CreateVertexShader(const DWORD* pFunction, IDirect3DVertexShader9** ppShader);
    HRESULT __stdcall SetVertexShader(IDirect3DVertexShader9* pShader);
    HRESULT __stdcall GetVertexShader(IDirect3DVertexShader9** ppShader);
    HRESULT __stdcall SetVertexShaderConstantF(unsigned int StartRegister, const float* pConstantData, unsigned int Vector4fCount);
    HRESULT __stdcall GetVertexShaderConstantF(unsigned int StartRegister, float* pConstantData, unsigned int Vector4fCount);
    HRESULT __stdcall SetVertexShaderConstantI(unsigned int StartRegister, const int* pConstantData, unsigned int Vector4iCount);
    HRESULT __stdcall GetVertexShaderConstantI(unsigned int StartRegister, int* pConstantData, unsigned int Vector4iCount);
    HRESULT __stdcall SetVertexShaderConstantB(unsigned int StartRegister, const BOOL* pConstantData, unsigned int BoolCount);
    HRESULT __stdcall GetVertexShaderConstantB(unsigned int StartRegister, BOOL* pConstantData, unsigned int BoolCount);
    HRESULT __stdcall SetStreamSource(
        unsigned int StreamNumber, IDirect3DVertexBuffer9* pStreamData, unsigned int OffsetInBytes, unsigned int Stride);
    HRESULT __stdcall GetStreamSource(
        unsigned int StreamNumber, IDirect3DVertexBuffer9** ppStreamData, unsigned int* pOffsetInBytes, unsigned int* pStride);
    HRESULT __stdcall SetStreamSourceFreq(unsigned int StreamNumber, unsigned int Setting);
    HRESULT __stdcall GetStreamSourceFreq(unsigned int StreamNumber, unsigned int* pSetting);
    HRESULT __stdcall SetIndices(IDirect3DIndexBuffer9* pIndexData);
    HRESULT __stdcall GetIndices(IDirect3DIndexBuffer9** ppIndexData);
    HRESULT __stdcall CreatePixelShader(const DWORD* pFunction, IDirect3DPixelShader9** ppShader);
    HRESULT __stdcall SetPixelShader(IDirect3DPixelShader9* pShader);
    HRESULT __stdcall GetPixelShader(IDirect3DPixelShader9** ppShader);
    HRESULT __stdcall SetPixelShaderConstantF(unsigned int StartRegister, const float* pConstantData, unsigned int Vector4fCount);
    HRESULT __stdcall GetPixelShaderConstantF(unsigned int StartRegister, float* pConstantData, unsigned int Vector4fCount);
    HRESULT __stdcall SetPixelShaderConstantI(unsigned int StartRegister, const int* pConstantData, unsigned int Vector4iCount);
    HRESULT __stdcall GetPixelShaderConstantI(unsigned int StartRegister, int* pConstantData, unsigned int Vector4iCount);
    HRESULT __stdcall SetPixelShaderConstantB(unsigned int StartRegister, const BOOL* pConstantData, unsigned int BoolCount);
    HRESULT __stdcall GetPixelShaderConstantB(unsigned int StartRegister, BOOL* pConstantData, unsigned int BoolCount);
    HRESULT __stdcall DrawRectPatch(unsigned int Handle, const float* pNumSegs, const D3DRECTPATCH_INFO* pRectPatchInfo);
    HRESULT __stdcall DrawTriPatch(unsigned int Handle, const float* pNumSegs, const D3DTRIPATCH_INFO* pTriPatchInfo);
    HRESULT __stdcall DeletePatch(unsigned int Handle);
    HRESULT __stdcall CreateQuery(D3DQUERYTYPE Type, IDirect3DQuery9** ppQuery);

    //#ifdef D3D_DEBUG_INFO
    D3DDEVICE_CREATION_PARAMETERS CreationParameters;
    D3DPRESENT_PARAMETERS PresentParameters;
    D3DDISPLAYMODE DisplayMode;
    D3DCAPS9 Caps;

    unsigned int AvailableTextureMem;
    unsigned int SwapChains;
    unsigned int Textures;
    unsigned int VertexBuffers;
    unsigned int IndexBuffers;
    unsigned int VertexShaders;
    unsigned int PixelShaders;

    D3DVIEWPORT9 Viewport;
    D3DMATRIX ProjectionMatrix;
    D3DMATRIX ViewMatrix;
    D3DMATRIX WorldMatrix;
    D3DMATRIX TextureMatrices[8];

    DWORD FVF;
    unsigned int VertexSize;
    DWORD VertexShaderVersion;
    DWORD PixelShaderVersion;
    BOOL SoftwareVertexProcessing;

    D3DMATERIAL9 Material;
    D3DLIGHT9 Lights[16];
    BOOL LightsEnabled[16];

    D3DGAMMARAMP GammaRamp;
    RECT ScissorRect;
    BOOL DialogBoxMode;
    //#endif
};

#ifdef __cplusplus
};
#endif
