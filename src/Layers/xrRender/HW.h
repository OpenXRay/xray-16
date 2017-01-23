// HW.h: interface for the CHW class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HW_H__0E25CF4A_FFEC_11D3_B4E3_4854E82A090D__INCLUDED_)
#define AFX_HW_H__0E25CF4A_FFEC_11D3_B4E3_4854E82A090D__INCLUDED_
#pragma once

#include "hwcaps.h"

#ifndef _MAYA_EXPORT
#include "stats_manager.h"
#endif

class  CHW
#if defined(USE_DX10) || defined(USE_DX11)
	:	public pureAppActivate, 
		public pureAppDeactivate
#endif	//	USE_DX10
{
//	Functions section
public:
	CHW();
	~CHW();

	void					CreateD3D				();
	void					DestroyD3D				();
	void					CreateDevice			(HWND hw, bool move_window);

	void					DestroyDevice			();

	void					Reset					(HWND hw);

	void					selectResolution		(u32 &dwWidth, u32 &dwHeight, BOOL bWindowed);
	D3DFORMAT				selectDepthStencil		(D3DFORMAT);
	u32						selectPresentInterval	();
	u32						selectGPU				();
	u32						selectRefresh			(u32 dwWidth, u32 dwHeight, D3DFORMAT fmt);
	void					updateWindowProps		(HWND hw);
	BOOL					support					(D3DFORMAT fmt, DWORD type, DWORD usage);

#ifdef DEBUG
#if defined(USE_DX10) || defined(USE_DX11)
	void	Validate(void)	{};
#else	//	USE_DX10
	void	Validate(void)	{	VERIFY(pDevice); VERIFY(pD3D); };
#endif	//	USE_DX10
#else
	void	Validate(void)	{};
#endif

//	Variables section
#if defined(USE_DX11)	//	USE_DX10
public:
	IDXGIAdapter*			m_pAdapter;	//	pD3D equivalent
	ID3D11Device*			pDevice;	//	combine with DX9 pDevice via typedef
	ID3D11DeviceContext*    pContext;	//	combine with DX9 pDevice via typedef
	IDXGISwapChain*         m_pSwapChain;
	ID3D11RenderTargetView*	pBaseRT;	//	combine with DX9 pBaseRT via typedef
	ID3D11DepthStencilView*	pBaseZB;

	CHWCaps					Caps;

	D3D_DRIVER_TYPE		m_DriverType;	//	DevT equivalent
	DXGI_SWAP_CHAIN_DESC	m_ChainDesc;	//	DevPP equivalent
	bool					m_bUsePerfhud;
	D3D_FEATURE_LEVEL		FeatureLevel;
#elif defined(USE_DX10)
public:
	IDXGIAdapter*			m_pAdapter;	//	pD3D equivalent
	ID3D10Device1*       	pDevice1;	//	combine with DX9 pDevice via typedef
	ID3D10Device*        	pDevice;	//	combine with DX9 pDevice via typedef
	ID3D10Device1*       	pContext1;	//	combine with DX9 pDevice via typedef
	ID3D10Device*        	pContext;	//	combine with DX9 pDevice via typedef
	IDXGISwapChain*         m_pSwapChain;
	ID3D10RenderTargetView*	pBaseRT;	//	combine with DX9 pBaseRT via typedef
	ID3D10DepthStencilView*	pBaseZB;

	CHWCaps					Caps;

	D3D10_DRIVER_TYPE		m_DriverType;	//	DevT equivalent
	DXGI_SWAP_CHAIN_DESC	m_ChainDesc;	//	DevPP equivalent
	bool					m_bUsePerfhud;
	D3D_FEATURE_LEVEL		FeatureLevel;
#else
private:
	HINSTANCE 				hD3D;

public:

	IDirect3D9* 			pD3D;		// D3D
	IDirect3DDevice9*		pDevice;	// render device

	IDirect3DSurface9*		pBaseRT;
	IDirect3DSurface9*		pBaseZB;

	CHWCaps					Caps;

	UINT					DevAdapter;
	D3DDEVTYPE				DevT;
	D3DPRESENT_PARAMETERS	DevPP;
#endif	//	USE_DX10

#ifndef _MAYA_EXPORT
	stats_manager			stats_manager;
#endif
#if defined(USE_DX10) || defined(USE_DX11)
	void			UpdateViews();
	DXGI_RATIONAL	selectRefresh(u32 dwWidth, u32 dwHeight, DXGI_FORMAT fmt);

	virtual	void	OnAppActivate();
	virtual void	OnAppDeactivate();
#endif	//	USE_DX10

private:
	bool					m_move_window;
};

extern ECORE_API CHW		HW;

#endif // !defined(AFX_HW_H__0E25CF4A_FFEC_11D3_B4E3_4854E82A090D__INCLUDED_)
