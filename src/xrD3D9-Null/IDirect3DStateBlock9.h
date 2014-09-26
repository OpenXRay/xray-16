

#ifdef __cplusplus
extern "C" {
#endif

	class xrIDirect3DStateBlock9: public IDirect3DStateBlock9
	{
	protected:
		LONG		m_refCount;
		IDirect3DDevice9*	m_pIDirect3DDevice9;
	public:
		xrIDirect3DStateBlock9(IDirect3DDevice9*	pIDirect3DDevice9);
		/*** IUnknown methods ***/
		HRESULT			__stdcall	QueryInterface(REFIID riid, void** ppvObj);
		ULONG			__stdcall	AddRef();
		ULONG			__stdcall	Release();

		/*** IDirect3DVertexDeclaration9 methods ***/
		HRESULT			__stdcall	GetDevice( IDirect3DDevice9** ppDevice);
		HRESULT			__stdcall	Capture();
		HRESULT			__stdcall	Apply();
	};

#ifdef __cplusplus
};
#endif