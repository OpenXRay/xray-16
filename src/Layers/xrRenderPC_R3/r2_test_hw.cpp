#include "stdafx.h"

BOOL	xrRender_test_hw		()
{
	CHW							_HW;
	HRESULT						hr;
	_HW.CreateD3D				()		;
	hr = _HW.m_pAdapter->CheckInterfaceSupport(__uuidof(ID3D10Device), 0);
	_HW.DestroyD3D				()		;

	return	SUCCEEDED(hr);
}
