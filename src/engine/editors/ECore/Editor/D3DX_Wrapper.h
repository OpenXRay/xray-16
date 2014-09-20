#ifndef D3DX_WraperH
#define D3DX_WraperH
#pragma once

#ifdef ETOOLS_EXPORTS
#define ETOOLS_API __declspec( dllexport )
#else
#define ETOOLS_API __declspec( dllimport )
#endif

extern "C" {
	ETOOLS_API UINT WINAPI
		D3DX_GetDriverLevel(
		LPDIRECT3DDEVICE9		pDevice);

	ETOOLS_API HRESULT WINAPI
		D3DX_GetImageInfoFromFileInMemory(
		LPCVOID					pSrcData,
		UINT					SrcDataSize,
		D3DXIMAGE_INFO*			pSrcInfo);

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
		LPDIRECT3DCUBETEXTURE9* ppCubeTexture);

	ETOOLS_API HRESULT WINAPI
		D3DX_CreateTextureFromFileInMemoryEx(
		LPDIRECT3DDEVICE9       pDevice,
		LPCVOID                 pSrcData,
		UINT                    SrcDataSize,
		UINT                    Width,
		UINT                    Height,
		UINT                    MipLevels,
		DWORD                   Usage,
		D3DFORMAT               Format,
		D3DPOOL                 Pool,
		DWORD                   Filter,
		DWORD                   MipFilter,
		D3DCOLOR                ColorKey,
		D3DXIMAGE_INFO*         pSrcInfo,
		PALETTEENTRY*           pPalette,
		LPDIRECT3DTEXTURE9*     ppTexture);

	ETOOLS_API HRESULT WINAPI
		D3DX_CreateTexture(
		LPDIRECT3DDEVICE9       pDevice,
		UINT                    Width,
		UINT                    Height,
		UINT                    MipLevels,
		DWORD                   Usage,
		D3DFORMAT               Format,
		D3DPOOL                 Pool,
		LPDIRECT3DTEXTURE9*     ppTexture);

	ETOOLS_API HRESULT WINAPI
		D3DX_ComputeNormalMap(
		LPDIRECT3DTEXTURE9 pTexture,
		LPDIRECT3DTEXTURE9 pSrcTexture,
		const PALETTEENTRY *pSrcPalette,
		DWORD Flags,
		DWORD Channel,
		FLOAT Amplitude);

	ETOOLS_API HRESULT WINAPI
		D3DX_LoadSurfaceFromSurface(
		LPDIRECT3DSURFACE9      pDestSurface,
		CONST PALETTEENTRY*     pDestPalette,
		CONST RECT*             pDestRect,
		LPDIRECT3DSURFACE9      pSrcSurface,
		CONST PALETTEENTRY*     pSrcPalette,
		CONST RECT*             pSrcRect,
		DWORD					Filter,
		D3DCOLOR				ColorKey);

	ETOOLS_API HRESULT WINAPI
		D3DX_CompileShader(
		LPCSTR                  pSrcData,
		UINT                    SrcDataLen,
		CONST D3DXMACRO*        pDefines,
		LPD3DXINCLUDE           pInclude,
		LPCSTR                  pFunctionName,
		LPCSTR                  pTarget,
		DWORD                   Flags,
		LPD3DXBUFFER*           ppShader,
		LPD3DXBUFFER*           ppErrorMsgs,
		LPD3DXCONSTANTTABLE*    ppConstantTable);

	ETOOLS_API HRESULT WINAPI
		D3DX_CompileShaderFromFile(
		LPCSTR                  pSrcFile,
		CONST D3DXMACRO*        pDefines,
		LPD3DXINCLUDE           pInclude,
		LPCSTR                  pFunctionName,
		LPCSTR                  pTarget,
		DWORD                   Flags,
		LPD3DXBUFFER*           ppShader,
		LPD3DXBUFFER*           ppErrorMsgs,
		LPD3DXCONSTANTTABLE*    ppConstantTable);

	ETOOLS_API HRESULT WINAPI
		D3DX_FindShaderComment(
		CONST DWORD*            pFunction,
		DWORD                   FourCC,
		LPCVOID*                ppData,
		UINT*                   pSizeInBytes);

	ETOOLS_API HRESULT WINAPI
		D3DX_DeclaratorFromFVF(
		DWORD FVF,
		D3DVERTEXELEMENT9 pDeclarator[MAX_FVF_DECL_SIZE]);

	ETOOLS_API UINT WINAPI
		D3DX_GetDeclVertexSize(
		CONST D3DVERTEXELEMENT9*pDecl,
		DWORD					Stream);

	ETOOLS_API UINT WINAPI
		D3DX_GetDeclLength(
		CONST D3DVERTEXELEMENT9*pDecl);

	ETOOLS_API UINT WINAPI
		D3DX_GetFVFVertexSize(
		DWORD					FVF);

	ETOOLS_API const char*  WINAPI
		DX_GetErrorDescription9(
		HRESULT					hr);

	ETOOLS_API D3DXMATRIX* WINAPI
		D3DX_MatrixInverse(          
		D3DXMATRIX *pOut,
		FLOAT *pDeterminant,
		CONST D3DXMATRIX *pM);

	ETOOLS_API D3DXMATRIX* WINAPI
		D3DX_MatrixTranspose(          
		D3DXMATRIX *pOut,
		CONST D3DXMATRIX *pM);

	ETOOLS_API D3DXPLANE* WINAPI
		D3DX_PlaneNormalize(          
		D3DXPLANE *pOut,
		CONST D3DXPLANE *pP);

	ETOOLS_API D3DXPLANE* WINAPI
		D3DX_PlaneTransform(          
		D3DXPLANE *pOut,
		CONST D3DXPLANE *pP,
		CONST D3DXMATRIX *pM);
        
	ETOOLS_API HRESULT WINAPI
    	D3DX_OptimizeFaces(
		LPCVOID pIndices,
      	UINT NumFaces,
      	UINT NumVertices,
      	BOOL Indices32Bit,
      	DWORD * pFaceRemap);

	ETOOLS_API HRESULT WINAPI
		D3DX_OptimizeVertices(
		LPCVOID pIndices,
		UINT NumFaces,
		UINT NumVertices,
		BOOL Indices32Bit,
		DWORD * pVertexRemap);
}

#ifndef ETOOLS_EXPORTS
	#undef D3DXCompileShaderFromFile
	#undef DXGetErrorDescription9
	#define D3DXGetDriverLevel						D3DX_GetDriverLevel
	#define D3DXGetImageInfoFromFileInMemory		D3DX_GetImageInfoFromFileInMemory
	#define D3DXCreateCubeTextureFromFileInMemoryEx	D3DX_CreateCubeTextureFromFileInMemoryEx
	#define D3DXCreateTextureFromFileInMemoryEx		D3DX_CreateTextureFromFileInMemoryEx
	#define D3DXCreateTexture						D3DX_CreateTexture
	#define D3DXComputeNormalMap					D3DX_ComputeNormalMap
	#define D3DXLoadSurfaceFromSurface				D3DX_LoadSurfaceFromSurface
	#define D3DXCompileShaderFromFile				D3DX_CompileShaderFromFile
	#define D3DXCompileShader						D3DX_CompileShader
	#define D3DXFindShaderComment					D3DX_FindShaderComment
	#define D3DXDeclaratorFromFVF					D3DX_DeclaratorFromFVF
	#define D3DXGetDeclVertexSize					D3DX_GetDeclVertexSize
	#define D3DXGetDeclLength						D3DX_GetDeclLength
	#define D3DXGetFVFVertexSize					D3DX_GetFVFVertexSize
	#define DXGetErrorDescription9					DX_GetErrorDescription9
	#define D3DXMatrixInverse						D3DX_MatrixInverse
	#define D3DXMatrixTranspose						D3DX_MatrixTranspose
	#define D3DXPlaneNormalize						D3DX_PlaneNormalize 
	#define D3DXPlaneTransform						D3DX_PlaneTransform 
	#define D3DXOptimizeFaces						D3DX_OptimizeFaces 
	#define D3DXOptimizeVertices					D3DX_OptimizeVertices 
#endif

#endif
