// CreateDX.cpp : Defines the entry point for the DLL application.
//
#include "stdafx.h"
#include "D3DX_Wrapper.h"
#include "xrCore/Debug/dxerr.h"

// misc
// Xottab_DUTY: it seems this is not used..
/*
__declspec(dllimport) bool WINAPI
FSColorPickerDoModal(unsigned int* currentColor, unsigned int* originalColor, const int initialExpansionState);
extern "C" __declspec(dllexport) bool __stdcall FSColorPickerExecute(
    unsigned int* currentColor, unsigned int* originalColor, const int initialExpansionState)
{
    return FSColorPickerDoModal(currentColor, originalColor, initialExpansionState);
}
*/
extern "C" {
ETOOLS_API unsigned int WINAPI D3DX_GetDriverLevel(LPDIRECT3DDEVICE9 pDevice) { return D3DXGetDriverLevel(pDevice); }
ETOOLS_API HRESULT WINAPI D3DX_GetImageInfoFromFileInMemory(
    const void* pSrcData, unsigned int SrcDataSize, D3DXIMAGE_INFO* pSrcInfo)
{
    return D3DXGetImageInfoFromFileInMemory(pSrcData, SrcDataSize, pSrcInfo);
}

ETOOLS_API HRESULT WINAPI D3DX_CreateCubeTextureFromFileInMemoryEx(LPDIRECT3DDEVICE9 pDevice, const void* pSrcData,
    unsigned int SrcDataSize, unsigned int Size, unsigned int MipLevels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, DWORD Filter,
    DWORD MipFilter, D3DCOLOR ColorKey, D3DXIMAGE_INFO* pSrcInfo, PALETTEENTRY* pPalette,
    LPDIRECT3DCUBETEXTURE9* ppCubeTexture)
{
    return D3DXCreateCubeTextureFromFileInMemoryEx(pDevice, pSrcData, SrcDataSize, Size, MipLevels, Usage, Format, Pool,
        Filter, MipFilter, ColorKey, pSrcInfo, pPalette, ppCubeTexture);
}

ETOOLS_API HRESULT WINAPI D3DX_CreateTextureFromFileInMemoryEx(LPDIRECT3DDEVICE9 pDevice, const void* pSrcData,
    unsigned int SrcDataSize, unsigned int Width, unsigned int Height, unsigned int MipLevels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool,
    DWORD Filter, DWORD MipFilter, D3DCOLOR ColorKey, D3DXIMAGE_INFO* pSrcInfo, PALETTEENTRY* pPalette,
    LPDIRECT3DTEXTURE9* ppTexture)
{
    return D3DXCreateTextureFromFileInMemoryEx(pDevice, pSrcData, SrcDataSize, Width, Height, MipLevels, Usage, Format,
        Pool, Filter, MipFilter, ColorKey, pSrcInfo, pPalette, ppTexture);
}

ETOOLS_API HRESULT WINAPI D3DX_CreateTexture(LPDIRECT3DDEVICE9 pDevice, unsigned int Width, unsigned int Height, unsigned int MipLevels,
    DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, LPDIRECT3DTEXTURE9* ppTexture)
{
    return D3DXCreateTexture(pDevice, Width, Height, MipLevels, Usage, Format, Pool, ppTexture);
}

ETOOLS_API HRESULT WINAPI D3DX_ComputeNormalMap(LPDIRECT3DTEXTURE9 pTexture, LPDIRECT3DTEXTURE9 pSrcTexture,
    const PALETTEENTRY* pSrcPalette, DWORD Flags, DWORD Channel, float Amplitude)
{
    return D3DXComputeNormalMap(pTexture, pSrcTexture, pSrcPalette, Flags, Channel, Amplitude);
}

ETOOLS_API HRESULT WINAPI D3DX_LoadSurfaceFromSurface(LPDIRECT3DSURFACE9 pDestSurface, const PALETTEENTRY* pDestPalette,
    const RECT* pDestRect, LPDIRECT3DSURFACE9 pSrcSurface, const PALETTEENTRY* pSrcPalette, const RECT* pSrcRect,
    DWORD Filter, D3DCOLOR ColorKey)
{
    return D3DXLoadSurfaceFromSurface(
        pDestSurface, pDestPalette, pDestRect, pSrcSurface, pSrcPalette, pSrcRect, Filter, ColorKey);
}

ETOOLS_API HRESULT WINAPI D3DX_CompileShader(const char* pSrcData, unsigned int SrcDataLen, const D3DXMACRO* pDefines,
    LPD3DXINCLUDE pInclude, const char* pFunctionName, const char* pTarget, DWORD Flags, LPD3DXBUFFER* ppShader,
    LPD3DXBUFFER* ppErrorMsgs, LPD3DXCONSTANTTABLE* ppConstantTable)
{
    return D3DXCompileShader(pSrcData, SrcDataLen, pDefines, pInclude, pFunctionName, pTarget, Flags, ppShader,
        ppErrorMsgs, ppConstantTable);
}

ETOOLS_API HRESULT WINAPI D3DX_CompileShaderFromFile(const char* pSrcFile, const D3DXMACRO* pDefines, LPD3DXINCLUDE pInclude,
    const char* pFunctionName, const char* pTarget, DWORD Flags, LPD3DXBUFFER* ppShader, LPD3DXBUFFER* ppErrorMsgs,
    LPD3DXCONSTANTTABLE* ppConstantTable)
{
    return D3DXCompileShaderFromFile(
        pSrcFile, pDefines, pInclude, pFunctionName, pTarget, Flags, ppShader, ppErrorMsgs, ppConstantTable);
}

ETOOLS_API HRESULT WINAPI D3DX_FindShaderComment(
    const DWORD* pFunction, DWORD FourCC, const void** ppData, unsigned int* pSizeInBytes)
{
    return D3DXFindShaderComment(pFunction, FourCC, ppData, pSizeInBytes);
}

ETOOLS_API HRESULT WINAPI D3DX_DeclaratorFromFVF(DWORD FVF, D3DVERTEXELEMENT9 pDeclarator[MAX_FVF_DECL_SIZE])
{
    return D3DXDeclaratorFromFVF(FVF, pDeclarator);
}

ETOOLS_API unsigned int WINAPI D3DX_GetDeclVertexSize(const D3DVERTEXELEMENT9* pDecl, DWORD Stream)
{
    return D3DXGetDeclVertexSize(pDecl, Stream);
}

ETOOLS_API unsigned int WINAPI D3DX_GetDeclLength(const D3DVERTEXELEMENT9* pDecl) { return D3DXGetDeclLength(pDecl); }
ETOOLS_API unsigned int WINAPI D3DX_GetFVFVertexSize(DWORD FVF) { return D3DXGetFVFVertexSize(FVF); }
ETOOLS_API const char* WINAPI DX_GetErrorDescription(HRESULT hr)
{
    static char desc[1024];
    DXGetErrorDescription(hr, desc, sizeof(desc));
    return desc;
}
ETOOLS_API D3DXMATRIX* WINAPI D3DX_MatrixInverse(D3DXMATRIX* pOut, float* pDeterminant, const D3DXMATRIX* pM)
{
    return D3DXMatrixInverse(pOut, pDeterminant, pM);
}

ETOOLS_API D3DXMATRIX* WINAPI D3DX_MatrixTranspose(D3DXMATRIX* pOut, const D3DXMATRIX* pM)
{
    return D3DXMatrixTranspose(pOut, pM);
}

ETOOLS_API D3DXPLANE* WINAPI D3DX_PlaneNormalize(D3DXPLANE* pOut, const D3DXPLANE* pP)
{
    return D3DXPlaneNormalize(pOut, pP);
}

ETOOLS_API D3DXPLANE* WINAPI D3DX_PlaneTransform(D3DXPLANE* pOut, const D3DXPLANE* pP, const D3DXMATRIX* pM)
{
    return D3DXPlaneTransform(pOut, pP, pM);
}

ETOOLS_API HRESULT WINAPI D3DX_OptimizeFaces(
    const void* pIndices, unsigned int NumFaces, unsigned int NumVertices, bool Indices32Bit, DWORD* pFaceRemap)
{
    return D3DXOptimizeFaces(pIndices, NumFaces, NumVertices, Indices32Bit, pFaceRemap);
}

ETOOLS_API HRESULT WINAPI D3DX_OptimizeVertices(
    const void* pIndices, unsigned int NumFaces, unsigned int NumVertices, bool Indices32Bit, DWORD* pVertexRemap)
{
    return D3DXOptimizeVertices(pIndices, NumFaces, NumVertices, Indices32Bit, pVertexRemap);
}
}
