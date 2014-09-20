//---------------------------------


#ifdef __cplusplus
extern "C" {
#endif


	class xrIDirect3DQuery9: public IDirect3DQuery9
	{
	protected:

		LONG		m_refCount;
		IDirect3DDevice9*	m_pIDirect3DDevice9;
	public:
		xrIDirect3DQuery9(IDirect3DDevice9* pIDirect3DDevice9, D3DQUERYTYPE rType);
		/*** IUnknown methods ***/
		HRESULT		__stdcall	QueryInterface( REFIID riid, void** ppvObj) ;
		ULONG		__stdcall	AddRef() ;
		ULONG		__stdcall	Release() ;

		/*** IDirect3DQuery9 methods ***/
		HRESULT			__stdcall	GetDevice( IDirect3DDevice9** ppDevice) ;
		D3DQUERYTYPE	__stdcall	GetType() ;
		DWORD			__stdcall	GetDataSize() ;
		HRESULT			__stdcall	Issue( DWORD dwIssueFlags) ;
		HRESULT			__stdcall	GetData( void* pData,DWORD dwSize,DWORD dwGetDataFlags) ;

//#ifdef D3D_DEBUG_INFO
		D3DQUERYTYPE Type;
		DWORD DataSize;
		LPCWSTR CreationCallStack;
//#endif
	};


#ifdef __cplusplus
};
#endif
