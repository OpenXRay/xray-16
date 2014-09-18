// CreateDX.cpp : Defines the entry point for the DLL application.
//

#include	"stdafx.h"
#include	"D3DX_Wrapper.h"

#include	"dxerr.h"
// misc
__declspec( dllimport ) bool WINAPI FSColorPickerDoModal(unsigned int * currentColor, unsigned int * originalColor, const int initialExpansionState);
extern "C" __declspec(dllexport) bool  __stdcall FSColorPickerExecute(unsigned int * currentColor, unsigned int * originalColor, const int initialExpansionState)
{
	return FSColorPickerDoModal(currentColor, originalColor, initialExpansionState);
}

extern "C"{ 
	ETOOLS_API UINT WINAPI D3DX_GetDriverLevel(LPDIRECT3DDEVICE9 pDevice)
	{
		return D3DXGetDriverLevel(pDevice);
	}
	
	ETOOLS_API HRESULT WINAPI	
		D3DX_GetImageInfoFromFileInMemory(
		LPCVOID					pSrcData, 
		UINT					SrcDataSize, 
		D3DXIMAGE_INFO*			pSrcInfo)
	{
		return D3DXGetImageInfoFromFileInMemory(pSrcData, SrcDataSize, pSrcInfo);
	}
	
	ETOOLS_API HRESULT WINAPI
		D3DX_CreateCubeTextureFromFileInMemoryEx(
		LPDIRECT3DDEVICE9       pDevice,
		LPCVOID                 pSrcData,
		UINT                    SrcDataSize,
		UINT                    Size,
		UINT                    MipLevels,
		DWORD                   Usage,
		D3DFORMAT               Format,
		D3DPOOL                 Pool,
		DWORD                   Filter,
		DWORD                   MipFilter,
		D3DCOLOR                ColorKey,
		D3DXIMAGE_INFO*         pSrcInfo,
		PALETTEENTRY*           pPalette,
		LPDIRECT3DCUBETEXTURE9* ppCubeTexture)
	{
		return D3DXCreateCubeTextureFromFileInMemoryEx(
			pDevice, pSrcData, SrcDataSize, Size, MipLevels, Usage,
			Format, Pool, Filter, MipFilter, ColorKey, pSrcInfo, pPalette,
			ppCubeTexture);
	}

	ETOOLS_API HRESULT WINAPI
		D3DX_CreateTextureFromFileInMemoryEx(
		LPDIRECT3DDEVICE9         pDevice,
		LPCVOID                   pSrcData,
		UINT                      SrcDataSize,
		UINT                      Width,
		UINT                      Height,
		UINT                      MipLevels,
		DWORD                     Usage,
		D3DFORMAT                 Format,
		D3DPOOL                   Pool,
		DWORD                     Filter,
		DWORD                     MipFilter,
		D3DCOLOR                  ColorKey,
		D3DXIMAGE_INFO*           pSrcInfo,
		PALETTEENTRY*             pPalette,
		LPDIRECT3DTEXTURE9*       ppTexture)
	{
		return D3DXCreateTextureFromFileInMemoryEx(
			pDevice, pSrcData, SrcDataSize, Width, Height, MipLevels, Usage,
			Format, Pool, Filter, MipFilter, ColorKey, pSrcInfo, pPalette, 
			ppTexture);
	}

	ETOOLS_API HRESULT WINAPI
		D3DX_CreateTexture(
		LPDIRECT3DDEVICE9         pDevice,
		UINT                      Width,
		UINT                      Height,
		UINT                      MipLevels,
		DWORD                     Usage,
		D3DFORMAT                 Format,
		D3DPOOL                   Pool,
		LPDIRECT3DTEXTURE9*       ppTexture)
	{
		return D3DXCreateTexture( pDevice, Width, Height, MipLevels, Usage, Format, Pool, ppTexture);
	}

	ETOOLS_API HRESULT WINAPI
		D3DX_ComputeNormalMap(
		LPDIRECT3DTEXTURE9 pTexture,
		LPDIRECT3DTEXTURE9 pSrcTexture,
		const PALETTEENTRY *pSrcPalette,
		DWORD Flags,
		DWORD Channel,
		FLOAT Amplitude)	
	{
		return D3DXComputeNormalMap( pTexture, pSrcTexture, pSrcPalette, Flags, Channel, Amplitude);
	}

	ETOOLS_API HRESULT WINAPI
		D3DX_LoadSurfaceFromSurface(
		LPDIRECT3DSURFACE9        pDestSurface,
		CONST PALETTEENTRY*       pDestPalette,
		CONST RECT*               pDestRect,
		LPDIRECT3DSURFACE9        pSrcSurface,
		CONST PALETTEENTRY*       pSrcPalette,
		CONST RECT*               pSrcRect,
		DWORD                     Filter,
		D3DCOLOR                  ColorKey)
	{
		return D3DXLoadSurfaceFromSurface(pDestSurface, pDestPalette, pDestRect, pSrcSurface, pSrcPalette, pSrcRect, Filter, ColorKey);
	}

	ETOOLS_API HRESULT WINAPI
		D3DX_CompileShader(
		LPCSTR                          pSrcData,
		UINT                            SrcDataLen,
		CONST D3DXMACRO*                pDefines,
		LPD3DXINCLUDE                   pInclude,
		LPCSTR                          pFunctionName,
		LPCSTR                          pTarget,
		DWORD                           Flags,
		LPD3DXBUFFER*                   ppShader,
		LPD3DXBUFFER*                   ppErrorMsgs,
		LPD3DXCONSTANTTABLE*            ppConstantTable)
	{
		return D3DXCompileShader(
			pSrcData, SrcDataLen, pDefines, pInclude, pFunctionName,
			pTarget, Flags, ppShader, ppErrorMsgs, ppConstantTable);
	}

	ETOOLS_API HRESULT WINAPI
		D3DX_CompileShaderFromFile(
		LPCSTR                          pSrcFile,
		CONST D3DXMACRO*                pDefines,
		LPD3DXINCLUDE                   pInclude,
		LPCSTR                          pFunctionName,
		LPCSTR                          pTarget,
		DWORD                           Flags,
		LPD3DXBUFFER*                   ppShader,
		LPD3DXBUFFER*                   ppErrorMsgs,
		LPD3DXCONSTANTTABLE*            ppConstantTable)
	{
		return D3DXCompileShaderFromFile(
			pSrcFile, pDefines, pInclude, pFunctionName, pTarget,
			Flags, ppShader, ppErrorMsgs, ppConstantTable);
	}

	ETOOLS_API HRESULT WINAPI
		D3DX_FindShaderComment(
		CONST DWORD*                    pFunction,
		DWORD                           FourCC,
		LPCVOID*                        ppData,
		UINT*                           pSizeInBytes)
	{
		return D3DXFindShaderComment(pFunction, FourCC, ppData, pSizeInBytes);
	}

	ETOOLS_API HRESULT WINAPI
		D3DX_DeclaratorFromFVF(
		DWORD							FVF,
		D3DVERTEXELEMENT9				pDeclarator[MAX_FVF_DECL_SIZE])
	{
		return D3DXDeclaratorFromFVF(FVF,pDeclarator);
	}

	ETOOLS_API UINT WINAPI 
		D3DX_GetDeclVertexSize(
		CONST D3DVERTEXELEMENT9*		pDecl,
		DWORD							Stream)
	{
		return D3DXGetDeclVertexSize(pDecl,Stream);
	}

	ETOOLS_API UINT WINAPI 
		D3DX_GetDeclLength(
		CONST D3DVERTEXELEMENT9 *pDecl)
	{
		return D3DXGetDeclLength(pDecl);
	}

	ETOOLS_API UINT WINAPI
		D3DX_GetFVFVertexSize(DWORD FVF)
	{
		return D3DXGetFVFVertexSize(FVF);
	}

	ETOOLS_API const char*  WINAPI DX_GetErrorDescription(HRESULT hr)
	{
		return DXGetErrorDescription(hr);
	}
	ETOOLS_API D3DXMATRIX* WINAPI 
		D3DX_MatrixInverse(          
		D3DXMATRIX *pOut,
		FLOAT *pDeterminant,
		CONST D3DXMATRIX *pM)
	{
		return D3DXMatrixInverse(pOut, pDeterminant,pM);
	}

	ETOOLS_API D3DXMATRIX* WINAPI
		D3DX_MatrixTranspose(          
		D3DXMATRIX *pOut,
		CONST D3DXMATRIX *pM)
	{
		return D3DXMatrixTranspose(pOut, pM);
	}

	ETOOLS_API D3DXPLANE* WINAPI
		D3DX_PlaneNormalize(          
		D3DXPLANE *pOut,
		CONST D3DXPLANE *pP)
	{
		return D3DXPlaneNormalize(pOut, pP);
	}

	ETOOLS_API D3DXPLANE* WINAPI
		D3DX_PlaneTransform(          
		D3DXPLANE *pOut,
		CONST D3DXPLANE *pP,
		CONST D3DXMATRIX *pM)
	{
		return D3DXPlaneTransform(pOut, pP, pM);
	}

	ETOOLS_API HRESULT WINAPI
		D3DX_OptimizeFaces(
		LPCVOID pIndices,
		UINT NumFaces,
		UINT NumVertices,
		BOOL Indices32Bit,
		DWORD * pFaceRemap)
	{
		return D3DXOptimizeFaces(pIndices, NumFaces, NumVertices, Indices32Bit, pFaceRemap);
	}

	ETOOLS_API HRESULT WINAPI
		D3DX_OptimizeVertices(
		LPCVOID pIndices,
		UINT NumFaces,
		UINT NumVertices,
		BOOL Indices32Bit,
		DWORD * pVertexRemap)
	{
		return D3DXOptimizeVertices(pIndices, NumFaces, NumVertices, Indices32Bit, pVertexRemap);
	}
}
