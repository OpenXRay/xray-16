

#ifdef __cplusplus
extern "C" {
#endif

	class xrIDirect3DCubeTexture9: public IDirect3DCubeTexture9
	{
	protected:

		LONG		m_refCount;
		IDirect3DDevice9*	m_pIDirect3DDevice9;
	public:
		xrIDirect3DCubeTexture9(IDirect3DDevice9*	pIDirect3DDevice9, UINT iWidth,UINT iHeight,UINT iLevels,DWORD iUsage,D3DFORMAT iFormat,D3DPOOL iPool);
		/*** IUnknown methods ***/
		HRESULT			__stdcall	QueryInterface( REFIID riid, void** ppvObj) ;
		ULONG			__stdcall	AddRef() ;
		ULONG			__stdcall	Release() ;

		/*** IDirect3DBaseTexture9 methods ***/
		HRESULT			__stdcall	GetDevice( IDirect3DDevice9** ppDevice) ;
		HRESULT			__stdcall	SetPrivateData( REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags) ;
		HRESULT			__stdcall	GetPrivateData( REFGUID refguid,void* pData,DWORD* pSizeOfData) ;
		HRESULT			__stdcall	FreePrivateData( REFGUID refguid) ;
		DWORD			__stdcall	SetPriority( DWORD PriorityNew) ;
		DWORD			__stdcall	GetPriority() ;
		void			__stdcall	PreLoad() ;
		D3DRESOURCETYPE	__stdcall	GetType() ;
		DWORD			__stdcall	SetLOD( DWORD LODNew) ;
		DWORD			__stdcall	GetLOD() ;
		DWORD			__stdcall	GetLevelCount() ;
		HRESULT			__stdcall	SetAutoGenFilterType( D3DTEXTUREFILTERTYPE FilterType) ;
		D3DTEXTUREFILTERTYPE			__stdcall	GetAutoGenFilterType() ;
		void			__stdcall	GenerateMipSubLevels() ;
		HRESULT			__stdcall	GetLevelDesc( UINT Level,D3DSURFACE_DESC *pDesc) ;
		HRESULT			__stdcall	GetCubeMapSurface( D3DCUBEMAP_FACES FaceType,UINT Level,IDirect3DSurface9** ppCubeMapSurface);
		HRESULT			__stdcall	LockRect( D3DCUBEMAP_FACES FaceType, UINT Level,D3DLOCKED_RECT* pLockedRect,CONST RECT* pRect,DWORD Flags) ;
		HRESULT			__stdcall	UnlockRect( D3DCUBEMAP_FACES FaceType, UINT Level) ;
		HRESULT			__stdcall	AddDirtyRect( D3DCUBEMAP_FACES FaceType, CONST RECT* pDirtyRect) ;

		//#ifdef D3D_DEBUG_INFO
		LPCWSTR Name;
		UINT Width;
		UINT Height;
		UINT Levels;
		DWORD Usage;
		D3DFORMAT Format;
		D3DPOOL Pool;
		DWORD Priority;
		DWORD LOD;
		D3DTEXTUREFILTERTYPE FilterType;
		UINT LockCount;
		LPCWSTR CreationCallStack;
		//#endif
	};

#ifdef __cplusplus
};
#endif