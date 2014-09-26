

#ifdef __cplusplus
extern "C" {
#endif

	class xrIDirect3DIndexBuffer9: public IDirect3DIndexBuffer9
	{
	protected:

		LONG		m_refCount;
		BYTE		*m_pBuffer;
		IDirect3DDevice9*	m_pIDirect3DDevice9;
	public:
		xrIDirect3DIndexBuffer9(IDirect3DDevice9*	pIDirect3DDevice9, UINT iLength,DWORD iUsage,D3DFORMAT iFormat,D3DPOOL iPool);
		/*** IUnknown methods ***/
		HRESULT			__stdcall	QueryInterface( REFIID riid, void** ppvObj) ;
		ULONG			__stdcall	AddRef() ;
		ULONG			__stdcall	Release() ;

		/*** IDirect3DResource9 methods ***/
		HRESULT			__stdcall	GetDevice( IDirect3DDevice9** ppDevice) ;
		HRESULT			__stdcall	SetPrivateData( REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags) ;
		HRESULT			__stdcall	GetPrivateData( REFGUID refguid,void* pData,DWORD* pSizeOfData) ;
		HRESULT			__stdcall	FreePrivateData( REFGUID refguid) ;
		DWORD			__stdcall	SetPriority( DWORD PriorityNew) ;
		DWORD			__stdcall	GetPriority() ;
		void			__stdcall	PreLoad() ;
		D3DRESOURCETYPE	__stdcall	GetType() ;
		HRESULT			__stdcall	Lock( UINT OffsetToLock,UINT SizeToLock,void** ppbData,DWORD Flags) ;
		HRESULT			__stdcall	Unlock() ;
		HRESULT			__stdcall	GetDesc( D3DINDEXBUFFER_DESC *pDesc) ;

//#ifdef D3D_DEBUG_INFO
		LPCWSTR Name;
		UINT Length;
		DWORD Usage;
		D3DFORMAT Format;
		D3DPOOL Pool;
		DWORD Priority;
		UINT LockCount;
		LPCWSTR CreationCallStack;
//#endif
	};

#ifdef __cplusplus
};
#endif