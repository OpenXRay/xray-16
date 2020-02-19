#ifndef D3DX_WraperH
#define D3DX_WraperH
#pragma once

#ifdef ETOOLS_EXPORTS
#define ETOOLS_API __declspec(dllexport)
#else
#define ETOOLS_API __declspec(dllimport)
#endif

extern "C" {
ETOOLS_API unsigned int WINAPI D3DX_GetDriverLevel(LPDIRECT3DDEVICE9 pDevice);

ETOOLS_API HRESULT WINAPI D3DX_GetImageInfoFromFileInMemory(
    const void* pSrcData, unsigned int SrcDataSize, D3DXIMAGE_INFO* pSrcInfo);

ETOOLS_API HRESULT WINAPI D3DX_CreateCubeTextureFromFileInMemoryEx(LPDIRECT3DDEVICE9 pDevice, const void* pSrcData,
    unsigned int SrcDataSize, unsigned int Size, unsigned int MipLevels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, DWORD Filter,
    DWORD MipFilter, D3DCOLOR ColorKey, D3DXIMAGE_INFO* pSrcInfo, PALETTEENTRY* pPalette,
    LPDIRECT3DCUBETEXTURE9* ppCubeTexture);

ETOOLS_API HRESULT WINAPI D3DX_CreateTextureFromFileInMemoryEx(LPDIRECT3DDEVICE9 pDevice, const void* pSrcData,
    unsigned int SrcDataSize, unsigned int Width, unsigned int Height, unsigned int MipLevels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool,
    DWORD Filter, DWORD MipFilter, D3DCOLOR ColorKey, D3DXIMAGE_INFO* pSrcInfo, PALETTEENTRY* pPalette,
    LPDIRECT3DTEXTURE9* ppTexture);

ETOOLS_API HRESULT WINAPI D3DX_CreateTexture(LPDIRECT3DDEVICE9 pDevice, unsigned int Width, unsigned int Height, unsigned int MipLevels,
    DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, LPDIRECT3DTEXTURE9* ppTexture);

ETOOLS_API HRESULT WINAPI D3DX_ComputeNormalMap(LPDIRECT3DTEXTURE9 pTexture, LPDIRECT3DTEXTURE9 pSrcTexture,
    const PALETTEENTRY* pSrcPalette, DWORD Flags, DWORD Channel, float Amplitude);

ETOOLS_API HRESULT WINAPI D3DX_LoadSurfaceFromSurface(LPDIRECT3DSURFACE9 pDestSurface, const PALETTEENTRY* pDestPalette,
    const RECT* pDestRect, LPDIRECT3DSURFACE9 pSrcSurface, const PALETTEENTRY* pSrcPalette, const RECT* pSrcRect,
    DWORD Filter, D3DCOLOR ColorKey);

ETOOLS_API HRESULT WINAPI D3DX_CompileShader(const char* pSrcData, unsigned int SrcDataLen, const D3DXMACRO* pDefines,
    LPD3DXINCLUDE pInclude, const char* pFunctionName, const char* pTarget, DWORD Flags, LPD3DXBUFFER* ppShader,
    LPD3DXBUFFER* ppErrorMsgs, LPD3DXCONSTANTTABLE* ppConstantTable);

ETOOLS_API HRESULT WINAPI D3DX_CompileShaderFromFile(const char* pSrcFile, const D3DXMACRO* pDefines, LPD3DXINCLUDE pInclude,
    const char* pFunctionName, const char* pTarget, DWORD Flags, LPD3DXBUFFER* ppShader, LPD3DXBUFFER* ppErrorMsgs,
    LPD3DXCONSTANTTABLE* ppConstantTable);

ETOOLS_API HRESULT WINAPI D3DX_FindShaderComment(
    const DWORD* pFunction, DWORD FourCC, const void** ppData, unsigned int* pSizeInBytes);

ETOOLS_API HRESULT WINAPI D3DX_DeclaratorFromFVF(DWORD FVF, D3DVERTEXELEMENT9 pDeclarator[MAX_FVF_DECL_SIZE]);

ETOOLS_API unsigned int WINAPI D3DX_GetDeclVertexSize(const D3DVERTEXELEMENT9* pDecl, DWORD Stream);

ETOOLS_API unsigned int WINAPI D3DX_GetDeclLength(const D3DVERTEXELEMENT9* pDecl);

ETOOLS_API unsigned int WINAPI D3DX_GetFVFVertexSize(DWORD FVF);

ETOOLS_API const char* WINAPI DX_GetErrorDescription(HRESULT hr);

ETOOLS_API D3DXMATRIX* WINAPI D3DX_MatrixInverse(D3DXMATRIX* pOut, float* pDeterminant, const D3DXMATRIX* pM);

ETOOLS_API D3DXMATRIX* WINAPI D3DX_MatrixTranspose(D3DXMATRIX* pOut, const D3DXMATRIX* pM);

ETOOLS_API D3DXPLANE* WINAPI D3DX_PlaneNormalize(D3DXPLANE* pOut, const D3DXPLANE* pP);

ETOOLS_API D3DXPLANE* WINAPI D3DX_PlaneTransform(D3DXPLANE* pOut, const D3DXPLANE* pP, const D3DXMATRIX* pM);

ETOOLS_API HRESULT WINAPI D3DX_OptimizeFaces(
    const void* pIndices, unsigned int NumFaces, unsigned int NumVertices, bool Indices32Bit, DWORD* pFaceRemap);

ETOOLS_API HRESULT WINAPI D3DX_OptimizeVertices(
    const void* pIndices, unsigned int NumFaces, unsigned int NumVertices, bool Indices32Bit, DWORD* pVertexRemap);
}

#ifndef ETOOLS_EXPORTS
#undef D3DXCompileShaderFromFile
#undef DXGetErrorDescription
#define D3DXGetDriverLevel D3DX_GetDriverLevel
#define D3DXGetImageInfoFromFileInMemory D3DX_GetImageInfoFromFileInMemory
#define D3DXCreateCubeTextureFromFileInMemoryEx D3DX_CreateCubeTextureFromFileInMemoryEx
#define D3DXCreateTextureFromFileInMemoryEx D3DX_CreateTextureFromFileInMemoryEx
#define D3DXCreateTexture D3DX_CreateTexture
#define D3DXComputeNormalMap D3DX_ComputeNormalMap
#define D3DXLoadSurfaceFromSurface D3DX_LoadSurfaceFromSurface
#define D3DXCompileShaderFromFile D3DX_CompileShaderFromFile
#define D3DXCompileShader D3DX_CompileShader
#define D3DXFindShaderComment D3DX_FindShaderComment
#define D3DXDeclaratorFromFVF D3DX_DeclaratorFromFVF
#define D3DXGetDeclVertexSize D3DX_GetDeclVertexSize
#define D3DXGetDeclLength D3DX_GetDeclLength
#define D3DXGetFVFVertexSize D3DX_GetFVFVertexSize
#define DXGetErrorDescription DX_GetErrorDescription
#define D3DXMatrixInverse D3DX_MatrixInverse
#define D3DXMatrixTranspose D3DX_MatrixTranspose
#define D3DXPlaneNormalize D3DX_PlaneNormalize
#define D3DXPlaneTransform D3DX_PlaneTransform
#define D3DXOptimizeFaces D3DX_OptimizeFaces
#define D3DXOptimizeVertices D3DX_OptimizeVertices
#endif

#endif
