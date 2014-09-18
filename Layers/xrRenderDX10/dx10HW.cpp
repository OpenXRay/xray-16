// dx10HW.cpp: implementation of the DX10 specialisation of CHW.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#pragma warning(disable:4995)
#include <d3dx9.h>
#pragma warning(default:4995)
#include "../xrRender/HW.h"
#include "../../xrEngine/XR_IOConsole.h"
#include "../../Include/xrAPI/xrAPI.h"

#include "StateManager\dx10SamplerStateCache.h"
#include "StateManager\dx10StateCache.h"

#ifndef _EDITOR
void	fill_vid_mode_list			(CHW* _hw);
void	free_vid_mode_list			();

void	fill_render_mode_list		();
void	free_render_mode_list		();
#else
void	fill_vid_mode_list			(CHW* _hw)	{}
void	free_vid_mode_list			()			{}
void	fill_render_mode_list		()			{}
void	free_render_mode_list		()			{}
#endif

CHW			HW;

//	DX10: Don't neeed this?
/*
#ifdef DEBUG
IDirect3DStateBlock9*	dwDebugSB = 0;
#endif
*/

CHW::CHW() : 
//	hD3D(NULL),
	//pD3D(NULL),
	m_pAdapter(0),
	pDevice(NULL),
	m_move_window(true)
	//pBaseRT(NULL),
	//pBaseZB(NULL)
{
	Device.seqAppActivate.Add(this);
	Device.seqAppDeactivate.Add(this);
}

CHW::~CHW()
{
	Device.seqAppActivate.Remove(this);
	Device.seqAppDeactivate.Remove(this);
}
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
void CHW::CreateD3D()
{
	/*	Partially implemented dynamic load
	LPCSTR		_name			= "d3d10.dll";

	hD3D            			= LoadLibrary(_name);

	//	If library can't be loaded computer don't support DirectX 10 at all
	if (!hD3D)					return;
	//	check if adapter support Direc3D 10 interface

	typedef HRESULT _CreateDXGIFactory( REFIID riid,	void **ppFactory);

	_CreateDXGIFactory  *CreateFactory = (_CreateDXGIFactory*)GetProcAddress(hD3D,"CreateDXGIFactory");
	R_ASSERT(CreateFactory);

	IDXGIFactory * pFactory;
	R_CHK( CreateFactory(__uuidof(IDXGIFactory), (void**)(&pFactory)) );
	pFactory->EnumAdapters(0, &m_pAdapter);
	pFactory->Release();
	*/

	IDXGIFactory * pFactory;
	R_CHK( CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)(&pFactory)) );

	m_pAdapter = 0;
	m_bUsePerfhud = false;

#ifndef	MASTER_GOLD
	// Look for 'NVIDIA NVPerfHUD' adapter
	// If it is present, override default settings
	UINT i = 0;
	while(pFactory->EnumAdapters(i, &m_pAdapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC desc;
		m_pAdapter->GetDesc(&desc);
		if(!wcscmp(desc.Description,L"NVIDIA PerfHUD"))
		{
			m_bUsePerfhud = true;
			break;
		}
		else
		{
			m_pAdapter->Release();
			m_pAdapter = 0;
		}
		++i;
	}
#endif	//	MASTER_GOLD

	if (!m_pAdapter)
		pFactory->EnumAdapters(0, &m_pAdapter);

	pFactory->Release();

	/*
	R_ASSERT2	           	 	(hD3D,"Can't find 'd3d10.dll'\nPlease install latest version of DirectX before running this program");
	typedef IDirect3D9 * WINAPI _Direct3DCreate9(UINT SDKVersion);
	_Direct3DCreate9* createD3D	= (_Direct3DCreate9*)GetProcAddress(hD3D,"Direct3DCreate9");	R_ASSERT(createD3D);
	this->pD3D 					= createD3D( D3D_SDK_VERSION );
	R_ASSERT2					(this->pD3D,"Please install DirectX 9.0c");
	*/
}

void CHW::DestroyD3D()
{
	//_RELEASE					(this->pD3D);

	_SHOW_REF				("refCount:m_pAdapter",m_pAdapter);
	_RELEASE				(m_pAdapter);

//	FreeLibrary(hD3D);
}

void CHW::CreateDevice( HWND m_hWnd, bool move_window )
{
	m_move_window			= move_window;
	CreateD3D();

	/* Partially implemented dynamic load
	typedef HRESULT _D3DxxCreateDeviceAndSwapChain(
		IDXGIAdapter *pAdapter,
		D3Dxx_DRIVER_TYPE DriverType,
		HMODULE Software,
		UINT Flags,
		UINT SDKVersion,
		DXGI_SWAP_CHAIN_DESC *pSwapChainDesc,
		IDXGISwapChain **ppSwapChain,
		ID3DxxDevice **ppDevice
		);



	_D3DxxCreateDeviceAndSwapChain *CreateDeviceAndSwapChain = 
		(_D3DxxCreateDeviceAndSwapChain*)
		GetProcAddress(hD3D,"D3DxxCreateDeviceAndSwapChain");
	R_ASSERT(CreateDeviceAndSwapChain);
	*/

	// TODO: DX10: Create appropriate initialization

	// General - select adapter and device
	BOOL  bWindowed			= !psDeviceFlags.is(rsFullscreen);

	m_DriverType = Caps.bForceGPU_REF ? 
		D3D_DRIVER_TYPE_REFERENCE : D3D_DRIVER_TYPE_HARDWARE;

	if (m_bUsePerfhud)
		m_DriverType = D3D_DRIVER_TYPE_REFERENCE;

	//	For DirectX 10 adapter is already created in create D3D.
	/*
	//. #ifdef DEBUG
	// Look for 'NVIDIA NVPerfHUD' adapter
	// If it is present, override default settings
	for (UINT Adapter=0;Adapter<pD3D->GetAdapterCount();Adapter++)	{
		D3DADAPTER_IDENTIFIER9 Identifier;
		HRESULT Res=pD3D->GetAdapterIdentifier(Adapter,0,&Identifier);
		if (SUCCEEDED(Res) && (xr_strcmp(Identifier.Description,"NVIDIA PerfHUD")==0))
		{
			DevAdapter	=Adapter;
			DevT		=D3DDEVTYPE_REF;
			break;
		}
	}
	//. #endif
	*/

	// Display the name of video board
	DXGI_ADAPTER_DESC Desc;
	R_CHK( m_pAdapter->GetDesc(&Desc) );
	//	Warning: Desc.Description is wide string
	Msg		("* GPU [vendor:%X]-[device:%X]: %S", Desc.VendorId, Desc.DeviceId, Desc.Description);
	/*
	// Display the name of video board
	D3DADAPTER_IDENTIFIER9	adapterID;
	R_CHK	(pD3D->GetAdapterIdentifier(DevAdapter,0,&adapterID));
	Msg		("* GPU [vendor:%X]-[device:%X]: %s",adapterID.VendorId,adapterID.DeviceId,adapterID.Description);

	u16	drv_Product		= HIWORD(adapterID.DriverVersion.HighPart);
	u16	drv_Version		= LOWORD(adapterID.DriverVersion.HighPart);
	u16	drv_SubVersion	= HIWORD(adapterID.DriverVersion.LowPart);
	u16	drv_Build		= LOWORD(adapterID.DriverVersion.LowPart);
	Msg		("* GPU driver: %d.%d.%d.%d",u32(drv_Product),u32(drv_Version),u32(drv_SubVersion), u32(drv_Build));
	*/

	/*
	Caps.id_vendor	= adapterID.VendorId;
	Caps.id_device	= adapterID.DeviceId;
	*/

	Caps.id_vendor	= Desc.VendorId;
	Caps.id_device	= Desc.DeviceId;

	/*
	// Retreive windowed mode
	D3DDISPLAYMODE mWindowed;
	R_CHK(pD3D->GetAdapterDisplayMode(DevAdapter, &mWindowed));

	*/
	// Select back-buffer & depth-stencil format
	D3DFORMAT&	fTarget	= Caps.fTarget;
	D3DFORMAT&	fDepth	= Caps.fDepth;

	//	HACK: DX10: Embed hard target format.
	fTarget = D3DFMT_X8R8G8B8;	//	No match in DX10. D3DFMT_A8B8G8R8->DXGI_FORMAT_R8G8B8A8_UNORM
	fDepth = selectDepthStencil(fTarget);
	/*
	if (bWindowed)
	{
		fTarget = mWindowed.Format;
		R_CHK(pD3D->CheckDeviceType	(DevAdapter,DevT,fTarget,fTarget,TRUE));
		fDepth  = selectDepthStencil(fTarget);
	} else {
		switch (psCurrentBPP) {
		case 32:
			fTarget = D3DFMT_X8R8G8B8;
			if (SUCCEEDED(pD3D->CheckDeviceType(DevAdapter,DevT,fTarget,fTarget,FALSE)))
				break;
			fTarget = D3DFMT_A8R8G8B8;
			if (SUCCEEDED(pD3D->CheckDeviceType(DevAdapter,DevT,fTarget,fTarget,FALSE)))
				break;
			fTarget = D3DFMT_R8G8B8;
			if (SUCCEEDED(pD3D->CheckDeviceType(DevAdapter,DevT,fTarget,fTarget,FALSE)))
				break;
			fTarget = D3DFMT_UNKNOWN;
			break;
		case 16:
		default:
			fTarget = D3DFMT_R5G6B5;
			if (SUCCEEDED(pD3D->CheckDeviceType(DevAdapter,DevT,fTarget,fTarget,FALSE)))
				break;
			fTarget = D3DFMT_X1R5G5B5;
			if (SUCCEEDED(pD3D->CheckDeviceType(DevAdapter,DevT,fTarget,fTarget,FALSE)))
				break;
			fTarget = D3DFMT_X4R4G4B4;
			if (SUCCEEDED(pD3D->CheckDeviceType(DevAdapter,DevT,fTarget,fTarget,FALSE)))
				break;
			fTarget = D3DFMT_UNKNOWN;
			break;
		}
		fDepth  = selectDepthStencil(fTarget);
	}
	

	if ((D3DFMT_UNKNOWN==fTarget) || (D3DFMT_UNKNOWN==fTarget))	{
		Msg					("Failed to initialize graphics hardware.\nPlease try to restart the game.");
		FlushLog			();
		MessageBox			(NULL,"Failed to initialize graphics hardware.\nPlease try to restart the game.","Error!",MB_OK|MB_ICONERROR);
		TerminateProcess	(GetCurrentProcess(),0);
	}

	*/
	
	// Set up the presentation parameters
	DXGI_SWAP_CHAIN_DESC	&sd	= m_ChainDesc;
	ZeroMemory				( &sd, sizeof(sd) );

	selectResolution	(sd.BufferDesc.Width, sd.BufferDesc.Height, bWindowed);

	// Back buffer
	//.	P.BackBufferWidth		= dwWidth;
	//. P.BackBufferHeight		= dwHeight;
	//	TODO: DX10: implement dynamic format selection
	//sd.BufferDesc.Format		= fTarget;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferCount = 1;

	// Multisample
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;

	// Windoze
	//P.SwapEffect			= bWindowed?D3DSWAPEFFECT_COPY:D3DSWAPEFFECT_DISCARD;
	//P.hDeviceWindow			= m_hWnd;
	//P.Windowed				= bWindowed;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.OutputWindow = m_hWnd;
	sd.Windowed = bWindowed;

	// Depth/stencil
	// DX10 don't need this?
	//P.EnableAutoDepthStencil= TRUE;
	//P.AutoDepthStencilFormat= fDepth;
	//P.Flags					= 0;	//. D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;

	// Refresh rate
	//P.PresentationInterval	= D3DPRESENT_INTERVAL_IMMEDIATE;
	//if( !bWindowed )		P.FullScreen_RefreshRateInHz	= selectRefresh	(P.BackBufferWidth, P.BackBufferHeight,fTarget);
	//else					P.FullScreen_RefreshRateInHz	= D3DPRESENT_RATE_DEFAULT;
	if (bWindowed)
	{
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
	}
	else
	{
		sd.BufferDesc.RefreshRate = selectRefresh( sd.BufferDesc.Width, sd.BufferDesc.Height, sd.BufferDesc.Format);
	}

	//	Additional set up
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	UINT createDeviceFlags = 0;
#ifdef DEBUG
	//createDeviceFlags |= D3Dxx_CREATE_DEVICE_DEBUG;
#endif
   HRESULT R;
	// Create the device
	//	DX10 don't need it?
	//u32 GPU		= selectGPU();
#ifdef USE_DX11
    D3D_FEATURE_LEVEL pFeatureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
//        D3D_FEATURE_LEVEL_10_1,
//        D3D_FEATURE_LEVEL_10_0,
    };

   R =  D3D11CreateDeviceAndSwapChain(   0,//m_pAdapter,//What wrong with adapter??? We should use another version of DXGI?????
                                          m_DriverType,
                                          NULL,
                                          createDeviceFlags,
										  pFeatureLevels,
										  sizeof(pFeatureLevels)/sizeof(pFeatureLevels[0]),
										  D3D11_SDK_VERSION,
                                          &sd,
                                          &m_pSwapChain,
		                                  &pDevice,
										  &FeatureLevel,		
										  &pContext);
#else
   R =  D3DX10CreateDeviceAndSwapChain(   m_pAdapter,
                                          m_DriverType,
                                          NULL,
                                          createDeviceFlags,
                                          &sd,
                                          &m_pSwapChain,
		                                    &pDevice );

   pContext = pDevice;
   FeatureLevel = D3D_FEATURE_LEVEL_10_0;
   if(!FAILED(R))
   {
      D3DX10GetFeatureLevel1( pDevice, &pDevice1 );
	  FeatureLevel = D3D_FEATURE_LEVEL_10_1;
   }
   pContext1 = pDevice1;
#endif

	/*
	if (FAILED(R))	{
		R	= HW.pD3D->CreateDevice(	DevAdapter,
			DevT,
			m_hWnd,
			GPU | D3DCREATE_MULTITHREADED,	//. ? locks at present
			&P,
			&pDevice );
	}
	*/
	//if (D3DERR_DEVICELOST==R)	{
	if (FAILED(R))
	{
		// Fatal error! Cannot create rendering device AT STARTUP !!!
		Msg					("Failed to initialize graphics hardware.\n"
							 "Please try to restart the game.\n"
							 "CreateDevice returned 0x%08x", R
							 );
		FlushLog			();
		MessageBox			(NULL,"Failed to initialize graphics hardware.\nPlease try to restart the game.","Error!",MB_OK|MB_ICONERROR);
		TerminateProcess	(GetCurrentProcess(),0);
	};
	R_CHK(R);

	_SHOW_REF	("* CREATE: DeviceREF:",HW.pDevice);
	/*
	switch (GPU)
	{
	case D3DCREATE_SOFTWARE_VERTEXPROCESSING:
		Log	("* Vertex Processor: SOFTWARE");
		break;
	case D3DCREATE_MIXED_VERTEXPROCESSING:
		Log	("* Vertex Processor: MIXED");
		break;
	case D3DCREATE_HARDWARE_VERTEXPROCESSING:
		Log	("* Vertex Processor: HARDWARE");
		break;
	case D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_PUREDEVICE:
		Log	("* Vertex Processor: PURE HARDWARE");
		break;
	}
	*/

	// Capture misc data
//	DX10: Don't neeed this?
//#ifdef DEBUG
//	R_CHK	(pDevice->CreateStateBlock			(D3DSBT_ALL,&dwDebugSB));
//#endif
	//	Create render target and depth-stencil views here
	UpdateViews();

	//u32	memory									= pDevice->GetAvailableTextureMem	();
	size_t	memory									= Desc.DedicatedVideoMemory;
	Msg		("*     Texture memory: %d M",		memory/(1024*1024));
	//Msg		("*          DDI-level: %2.1f",		float(D3DXGetDriverLevel(pDevice))/100.f);
#ifndef _EDITOR
	updateWindowProps							(m_hWnd);
	fill_vid_mode_list							(this);
#endif
}

void CHW::DestroyDevice()
{
	//	Destroy state managers
	StateManager.Reset();
	RSManager.ClearStateArray();
	DSSManager.ClearStateArray();
	BSManager.ClearStateArray();
	SSManager.ClearStateArray();

	_SHOW_REF				("refCount:pBaseZB",pBaseZB);
	_RELEASE				(pBaseZB);

	_SHOW_REF				("refCount:pBaseRT",pBaseRT);
	_RELEASE				(pBaseRT);
//#ifdef DEBUG
//	_SHOW_REF				("refCount:dwDebugSB",dwDebugSB);
//	_RELEASE				(dwDebugSB);
//#endif

	//	Must switch to windowed mode to release swap chain
	if (!m_ChainDesc.Windowed) m_pSwapChain->SetFullscreenState( FALSE, NULL);
	_SHOW_REF				("refCount:m_pSwapChain",m_pSwapChain);
	_RELEASE				(m_pSwapChain);

#ifdef USE_DX11
	_RELEASE				(pContext);
#endif

#ifndef USE_DX11
	_RELEASE				(HW.pDevice1);
#endif
	_SHOW_REF				("DeviceREF:",HW.pDevice);
	_RELEASE				(HW.pDevice);


	DestroyD3D				();

#ifndef _EDITOR
	free_vid_mode_list		();
#endif
}

//////////////////////////////////////////////////////////////////////
// Resetting device
//////////////////////////////////////////////////////////////////////
void CHW::Reset (HWND hwnd)
{
	DXGI_SWAP_CHAIN_DESC &cd = m_ChainDesc;

	BOOL	bWindowed		= !psDeviceFlags.is	(rsFullscreen);

	cd.Windowed = bWindowed;

	m_pSwapChain->SetFullscreenState(!bWindowed, NULL);

	DXGI_MODE_DESC	&desc = m_ChainDesc.BufferDesc;

	selectResolution(desc.Width, desc.Height, bWindowed);

	if (bWindowed)
	{
		desc.RefreshRate.Numerator = 60;
		desc.RefreshRate.Denominator = 1;
	}
	else
		desc.RefreshRate = selectRefresh( desc.Width, desc.Height, desc.Format);

	CHK_DX(m_pSwapChain->ResizeTarget(&desc));


#ifdef DEBUG
	//	_RELEASE			(dwDebugSB);
#endif
	_SHOW_REF				("refCount:pBaseZB",pBaseZB);
	_SHOW_REF				("refCount:pBaseRT",pBaseRT);

	_RELEASE(pBaseZB);
	_RELEASE(pBaseRT);

	CHK_DX(m_pSwapChain->ResizeBuffers(
		cd.BufferCount,
		desc.Width,
		desc.Height,
		desc.Format,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	UpdateViews();

/*
	// Windoze
	DevPP.SwapEffect			= bWindowed?D3DSWAPEFFECT_COPY:D3DSWAPEFFECT_DISCARD;
	DevPP.Windowed				= bWindowed;
	DevPP.PresentationInterval	= D3DPRESENT_INTERVAL_IMMEDIATE;
	if( !bWindowed )		DevPP.FullScreen_RefreshRateInHz	= selectRefresh	(DevPP.BackBufferWidth,DevPP.BackBufferHeight,Caps.fTarget);
	else					DevPP.FullScreen_RefreshRateInHz	= D3DPRESENT_RATE_DEFAULT;
#endif

	while	(TRUE)	{
		HRESULT _hr							= HW.pDevice->Reset	(&DevPP);
		if (SUCCEEDED(_hr))					break;
		Msg		("! ERROR: [%dx%d]: %s",DevPP.BackBufferWidth,DevPP.BackBufferHeight,Debug.error2string(_hr));
		Sleep	(100);
	}
	R_CHK				(pDevice->GetRenderTarget			(0,&pBaseRT));
	R_CHK				(pDevice->GetDepthStencilSurface	(&pBaseZB));
*/


//#ifdef DEBUG
//	R_CHK				(pDevice->CreateStateBlock			(D3DSBT_ALL,&dwDebugSB));
//#endif

	updateWindowProps	(hwnd);


		/*
#ifdef DEBUG
	_RELEASE			(dwDebugSB);
#endif
	_RELEASE			(pBaseZB);
	_RELEASE			(pBaseRT);

	BOOL	bWindowed		= !psDeviceFlags.is	(rsFullscreen);
#else
	BOOL	bWindowed		= TRUE;
#endif

	selectResolution		(DevPP.BackBufferWidth, DevPP.BackBufferHeight, bWindowed);
	// Windoze
	DevPP.SwapEffect			= bWindowed?D3DSWAPEFFECT_COPY:D3DSWAPEFFECT_DISCARD;
	DevPP.Windowed				= bWindowed;
	DevPP.PresentationInterval	= D3DPRESENT_INTERVAL_IMMEDIATE;
	if( !bWindowed )		DevPP.FullScreen_RefreshRateInHz	= selectRefresh	(DevPP.BackBufferWidth,DevPP.BackBufferHeight,Caps.fTarget);
	else					DevPP.FullScreen_RefreshRateInHz	= D3DPRESENT_RATE_DEFAULT;
#endif

	while	(TRUE)	{
		HRESULT _hr							= HW.pDevice->Reset	(&DevPP);
		if (SUCCEEDED(_hr))					break;
		Msg		("! ERROR: [%dx%d]: %s",DevPP.BackBufferWidth,DevPP.BackBufferHeight,Debug.error2string(_hr));
		Sleep	(100);
	}
	R_CHK				(pDevice->GetRenderTarget			(0,&pBaseRT));
	R_CHK				(pDevice->GetDepthStencilSurface	(&pBaseZB));
#ifdef DEBUG
	R_CHK				(pDevice->CreateStateBlock			(D3DSBT_ALL,&dwDebugSB));
#endif
#ifndef _EDITOR
	updateWindowProps	(hwnd);
#endif
	*/
}

D3DFORMAT CHW::selectDepthStencil	(D3DFORMAT fTarget)
{
	// R3 hack
#pragma todo("R3 need to specify depth format")
	return D3DFMT_D24S8;
}

void CHW::selectResolution( u32 &dwWidth, u32 &dwHeight, BOOL bWindowed )
{
	fill_vid_mode_list			(this);

	if(bWindowed)
	{
		dwWidth		= psCurrentVidMode[0];
		dwHeight	= psCurrentVidMode[1];
	}
	else //check
	{
		string64					buff;
		xr_sprintf					(buff,sizeof(buff),"%dx%d",psCurrentVidMode[0],psCurrentVidMode[1]);

		if(_ParseItem(buff,vid_mode_token)==u32(-1)) //not found
		{ //select safe
			xr_sprintf				(buff,sizeof(buff),"vid_mode %s",vid_mode_token[0].name);
			Console->Execute		(buff);
		}

		dwWidth						= psCurrentVidMode[0];
		dwHeight					= psCurrentVidMode[1];
	}
}

//	TODO: DX10: check if we need these
/*
u32	CHW::selectPresentInterval	()
{
	D3DCAPS9	caps;
	pD3D->GetDeviceCaps(DevAdapter,DevT,&caps);

	if (!psDeviceFlags.test(rsVSync)) 
	{
		if (caps.PresentationIntervals & D3DPRESENT_INTERVAL_IMMEDIATE)
			return D3DPRESENT_INTERVAL_IMMEDIATE;
		if (caps.PresentationIntervals & D3DPRESENT_INTERVAL_ONE)
			return D3DPRESENT_INTERVAL_ONE;
	}
	return D3DPRESENT_INTERVAL_DEFAULT;
}

u32 CHW::selectGPU ()
{
	if (Caps.bForceGPU_SW) return D3DCREATE_SOFTWARE_VERTEXPROCESSING;

	D3DCAPS9	caps;
	pD3D->GetDeviceCaps(DevAdapter,DevT,&caps);

	if(caps.DevCaps&D3DDEVCAPS_HWTRANSFORMANDLIGHT)
	{
		if (Caps.bForceGPU_NonPure)	return D3DCREATE_HARDWARE_VERTEXPROCESSING;
		else {
			if (caps.DevCaps&D3DDEVCAPS_PUREDEVICE) return D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_PUREDEVICE;
			else return D3DCREATE_HARDWARE_VERTEXPROCESSING;
		}
		// return D3DCREATE_MIXED_VERTEXPROCESSING;
	} else return D3DCREATE_SOFTWARE_VERTEXPROCESSING;
}
*/
DXGI_RATIONAL CHW::selectRefresh(u32 dwWidth, u32 dwHeight, DXGI_FORMAT fmt)
{
	DXGI_RATIONAL	res;

	res.Numerator = 60;
	res.Denominator = 1;

	float	CurrentFreq = 60.0f;

	if (psDeviceFlags.is(rsRefresh60hz))	
	{
		return res;
	}
	else
	{
		xr_vector<DXGI_MODE_DESC>	modes;

		IDXGIOutput *pOutput;
		m_pAdapter->EnumOutputs(0, &pOutput);
		VERIFY(pOutput);

		UINT num = 0;
		DXGI_FORMAT format = fmt;
		UINT flags         = 0;

		// Get the number of display modes available
		pOutput->GetDisplayModeList( format, flags, &num, 0);

		// Get the list of display modes
		modes.resize(num);
		pOutput->GetDisplayModeList( format, flags, &num, &modes.front());

		_RELEASE(pOutput);

		for (u32 i=0; i<num; ++i)
		{
			DXGI_MODE_DESC &desc = modes[i];

			if( (desc.Width == dwWidth) 
				&& (desc.Height == dwHeight)
				)
			{
				VERIFY(desc.RefreshRate.Denominator);
				float TempFreq = float(desc.RefreshRate.Numerator)/float(desc.RefreshRate.Denominator);
				if ( TempFreq > CurrentFreq )
				{
					CurrentFreq = TempFreq;
					res = desc.RefreshRate;
				}
			}
		}

		return res;
	}
}

void CHW::OnAppActivate()
{
	if ( m_pSwapChain && !m_ChainDesc.Windowed )
	{
		ShowWindow( m_ChainDesc.OutputWindow, SW_RESTORE );
		m_pSwapChain->SetFullscreenState( TRUE, NULL );
	}
}

void CHW::OnAppDeactivate()
{
	if ( m_pSwapChain && !m_ChainDesc.Windowed )
	{
		m_pSwapChain->SetFullscreenState( FALSE, NULL );
		ShowWindow( m_ChainDesc.OutputWindow, SW_MINIMIZE );
	}
}


BOOL CHW::support( D3DFORMAT fmt, DWORD type, DWORD usage)
{
	//	TODO: DX10: implement stub for this code.
	VERIFY(!"Implement CHW::support");
	/*
	HRESULT hr		= pD3D->CheckDeviceFormat(DevAdapter,DevT,Caps.fTarget,usage,(D3DRESOURCETYPE)type,fmt);
	if (FAILED(hr))	return FALSE;
	else			return TRUE;
	*/
	return TRUE;
}

void CHW::updateWindowProps(HWND m_hWnd)
{
	//	BOOL	bWindowed				= strstr(Core.Params,"-dedicated") ? TRUE : !psDeviceFlags.is	(rsFullscreen);
	BOOL	bWindowed				= !psDeviceFlags.is	(rsFullscreen);

	u32		dwWindowStyle			= 0;
	// Set window properties depending on what mode were in.
	if (bWindowed)		{
		if (m_move_window) {
			if (strstr(Core.Params,"-no_dialog_header"))
				SetWindowLong	( m_hWnd, GWL_STYLE, dwWindowStyle=(WS_BORDER|WS_VISIBLE) );
			else
				SetWindowLong	( m_hWnd, GWL_STYLE, dwWindowStyle=(WS_BORDER|WS_DLGFRAME|WS_VISIBLE|WS_SYSMENU|WS_MINIMIZEBOX ) );
			// When moving from fullscreen to windowed mode, it is important to
			// adjust the window size after recreating the device rather than
			// beforehand to ensure that you get the window size you want.  For
			// example, when switching from 640x480 fullscreen to windowed with
			// a 1000x600 window on a 1024x768 desktop, it is impossible to set
			// the window size to 1000x600 until after the display mode has
			// changed to 1024x768, because windows cannot be larger than the
			// desktop.

			RECT			m_rcWindowBounds;
			BOOL			bCenter = FALSE;
			if (strstr(Core.Params, "-center_screen"))	bCenter = TRUE;

			if (bCenter) {
				RECT				DesktopRect;

				GetClientRect		(GetDesktopWindow(), &DesktopRect);

				SetRect(			&m_rcWindowBounds, 
					(DesktopRect.right-m_ChainDesc.BufferDesc.Width)/2, 
					(DesktopRect.bottom-m_ChainDesc.BufferDesc.Height)/2, 
					(DesktopRect.right+m_ChainDesc.BufferDesc.Width)/2, 
					(DesktopRect.bottom+m_ChainDesc.BufferDesc.Height)/2);
			}else{
				SetRect(			&m_rcWindowBounds,
					0, 
					0, 
					m_ChainDesc.BufferDesc.Width, 
					m_ChainDesc.BufferDesc.Height);
			};

			AdjustWindowRect		(	&m_rcWindowBounds, dwWindowStyle, FALSE );

			SetWindowPos			(	m_hWnd, 
				HWND_NOTOPMOST,	
				m_rcWindowBounds.left, 
				m_rcWindowBounds.top,
				( m_rcWindowBounds.right - m_rcWindowBounds.left ),
				( m_rcWindowBounds.bottom - m_rcWindowBounds.top ),
				SWP_SHOWWINDOW|SWP_NOCOPYBITS|SWP_DRAWFRAME );
		}
	}
	else
	{
		SetWindowLong			( m_hWnd, GWL_STYLE, dwWindowStyle=(WS_POPUP|WS_VISIBLE) );
	}

	ShowCursor	(FALSE);
	SetForegroundWindow( m_hWnd );
}


struct _uniq_mode
{
	_uniq_mode(LPCSTR v):_val(v){}
	LPCSTR _val;
	bool operator() (LPCSTR _other) {return !stricmp(_val,_other);}
};

#ifndef _EDITOR

/*
void free_render_mode_list()
{
for( int i=0; vid_quality_token[i].name; i++ )
{
xr_free					(vid_quality_token[i].name);
}
xr_free						(vid_quality_token);
vid_quality_token			= NULL;
}
*/
/*
void	fill_render_mode_list()
{
if(vid_quality_token != NULL)		return;

D3DCAPS9					caps;
CHW							_HW;
_HW.CreateD3D				();
_HW.pD3D->GetDeviceCaps		(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,&caps);
_HW.DestroyD3D				();
u16		ps_ver_major		= u16 ( u32(u32(caps.PixelShaderVersion)&u32(0xf << 8ul))>>8 );

xr_vector<LPCSTR>			_tmp;
u32 i						= 0;
for(; i<5; ++i)
{
bool bBreakLoop = false;
switch (i)
{
case 3:		//"renderer_r2.5"
if (ps_ver_major < 3)
bBreakLoop = true;
break;
case 4:		//"renderer_r_dx10"
bBreakLoop = true;
break;
default:	;
}

if (bBreakLoop) break;

_tmp.push_back				(NULL);
LPCSTR val					= NULL;
switch (i)
{
case 0: val ="renderer_r1";			break;
case 1: val ="renderer_r2a";		break;
case 2: val ="renderer_r2";			break;
case 3: val ="renderer_r2.5";		break;
case 4: val ="renderer_r_dx10";		break; //  -)
}
_tmp.back()					= xr_strdup(val);
}
u32 _cnt								= _tmp.size()+1;
vid_quality_token						= xr_alloc<xr_token>(_cnt);

vid_quality_token[_cnt-1].id			= -1;
vid_quality_token[_cnt-1].name			= NULL;

#ifdef DEBUG
Msg("Available render modes[%d]:",_tmp.size());
#endif // DEBUG
for(u32 i=0; i<_tmp.size();++i)
{
vid_quality_token[i].id				= i;
vid_quality_token[i].name			= _tmp[i];
#ifdef DEBUG
Msg							("[%s]",_tmp[i]);
#endif // DEBUG
}
}
*/
void free_vid_mode_list()
{
	for( int i=0; vid_mode_token[i].name; i++ )
	{
		xr_free					(vid_mode_token[i].name);
	}
	xr_free						(vid_mode_token);
	vid_mode_token				= NULL;
}

void fill_vid_mode_list(CHW* _hw)
{
	if(vid_mode_token != NULL)		return;
	xr_vector<LPCSTR>	_tmp;
	xr_vector<DXGI_MODE_DESC>	modes;

	IDXGIOutput *pOutput;
	//_hw->m_pSwapChain->GetContainingOutput(&pOutput);
	_hw->m_pAdapter->EnumOutputs(0, &pOutput);
	VERIFY(pOutput);

	UINT num = 0;
	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
	UINT flags         = 0;

	// Get the number of display modes available
	pOutput->GetDisplayModeList( format, flags, &num, 0);

	// Get the list of display modes
	modes.resize(num);
	pOutput->GetDisplayModeList( format, flags, &num, &modes.front());

	_RELEASE(pOutput);

	for (u32 i=0; i<num; ++i)
	{
		DXGI_MODE_DESC &desc = modes[i];
		string32		str;

		if(desc.Width < 800)
			continue;

		xr_sprintf(str, sizeof(str), "%dx%d", desc.Width, desc.Height);

		if(_tmp.end() != std::find_if(_tmp.begin(), _tmp.end(), _uniq_mode(str)))
			continue;

		_tmp.push_back				(NULL);
		_tmp.back()					= xr_strdup(str);
	}
	


//	_tmp.push_back				(NULL);
//	_tmp.back()					= xr_strdup("1024x768");

	u32 _cnt						= _tmp.size()+1;

	vid_mode_token					= xr_alloc<xr_token>(_cnt);

	vid_mode_token[_cnt-1].id			= -1;
	vid_mode_token[_cnt-1].name		= NULL;

#ifdef DEBUG
	Msg("Available video modes[%d]:",_tmp.size());
#endif // DEBUG
	for( u32 i=0; i<_tmp.size(); ++i )
	{
		vid_mode_token[i].id		= i;
		vid_mode_token[i].name		= _tmp[i];
#ifdef DEBUG
		Msg							("[%s]",_tmp[i]);
#endif // DEBUG
	}

	/*	Old code
	if(vid_mode_token != NULL)		return;
	xr_vector<LPCSTR>	_tmp;
	u32 cnt = _hw->pD3D->GetAdapterModeCount	(_hw->DevAdapter, _hw->Caps.fTarget);

	u32 i;
	for(i=0; i<cnt;++i)
	{
		D3DDISPLAYMODE	Mode;
		string32		str;

		_hw->pD3D->EnumAdapterModes(_hw->DevAdapter, _hw->Caps.fTarget, i, &Mode);
		if(Mode.Width < 800)		continue;

		xr_sprintf						(str,sizeof(str),"%dx%d", Mode.Width, Mode.Height);

		if(_tmp.end() != std::find_if(_tmp.begin(), _tmp.end(), _uniq_mode(str)))
			continue;

		_tmp.push_back				(NULL);
		_tmp.back()					= xr_strdup(str);
	}

	u32 _cnt						= _tmp.size()+1;

	vid_mode_token					= xr_alloc<xr_token>(_cnt);

	vid_mode_token[_cnt-1].id			= -1;
	vid_mode_token[_cnt-1].name		= NULL;

#ifdef DEBUG
	Msg("Available video modes[%d]:",_tmp.size());
#endif // DEBUG
	for(i=0; i<_tmp.size();++i)
	{
		vid_mode_token[i].id		= i;
		vid_mode_token[i].name		= _tmp[i];
#ifdef DEBUG
		Msg							("[%s]",_tmp[i]);
#endif // DEBUG
	}
	*/
}

void CHW::UpdateViews()
{
	DXGI_SWAP_CHAIN_DESC &sd = m_ChainDesc;
	HRESULT R;

	// Create a render target view
	//R_CHK	(pDevice->GetRenderTarget			(0,&pBaseRT));
	ID3DTexture2D *pBuffer;
	R = m_pSwapChain->GetBuffer( 0, __uuidof( ID3DTexture2D ), (LPVOID*)&pBuffer );
	R_CHK(R);

	R = pDevice->CreateRenderTargetView( pBuffer, NULL, &pBaseRT);
	pBuffer->Release();
	R_CHK(R);

	//	Create Depth/stencil buffer
	//	HACK: DX10: hard depth buffer format
	//R_CHK	(pDevice->GetDepthStencilSurface	(&pBaseZB));
	ID3DTexture2D* pDepthStencil = NULL;
	D3D_TEXTURE2D_DESC descDepth;
	descDepth.Width = sd.BufferDesc.Width;
	descDepth.Height = sd.BufferDesc.Height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D_USAGE_DEFAULT;
	descDepth.BindFlags = D3D_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	R = pDevice->CreateTexture2D( &descDepth,       // Texture desc
		NULL,                  // Initial data
		&pDepthStencil ); // [out] Texture
	R_CHK(R);

	//	Create Depth/stencil view
	R = pDevice->CreateDepthStencilView( pDepthStencil, NULL, &pBaseZB );
	R_CHK(R);

	pDepthStencil->Release();
}
#endif
