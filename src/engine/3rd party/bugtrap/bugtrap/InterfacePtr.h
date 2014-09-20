/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Smart pointer to COM interface.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#pragma once

template <class INTERFACE>
class CInterfacePtr
{
public:
	CInterfacePtr(void) { m_ptr = NULL; }
	CInterfacePtr(int iNull) { _ASSERTE(iNull == 0); iNull; m_ptr = NULL; }
	CInterfacePtr(INTERFACE* ptr) { Assign(ptr); }
	CInterfacePtr(const CInterfacePtr& iptr) { Assign(iptr.m_ptr); }
	~CInterfacePtr(void) { Free(); }
	const CInterfacePtr& operator=(INTERFACE* ptr) { Free(); Assign(ptr); return *this; }
	const CInterfacePtr& operator=(const CInterfacePtr& iptr) { Free(); Assign(iptr.m_ptr); return *this; }

	INTERFACE** operator&(void) { _ASSERTE(m_ptr == NULL); return &m_ptr; }
	INTERFACE& operator*(void) const { _ASSERTE(m_ptr != NULL); return *m_ptr; }
	INTERFACE* operator->(void) const { _ASSERTE(m_ptr != NULL); return m_ptr; }
	bool operator!(void) const { return (m_ptr == NULL); }
	operator INTERFACE*(void) const { return m_ptr; }

	friend bool operator==(const CInterfacePtr& iptr1, const CInterfacePtr& iptr2) { return iptr1.m_ptr == iptr2.m_ptr; }
	friend bool operator!=(const CInterfacePtr& iptr1, const CInterfacePtr& iptr2) { return iptr1.m_ptr != iptr2.m_ptr; }
	friend bool operator<(const CInterfacePtr& iptr1, const CInterfacePtr& iptr2) { return iptr1.m_ptr < iptr2.m_ptr; }
	friend bool operator<=(const CInterfacePtr& iptr1, const CInterfacePtr& iptr2) { return iptr1.m_ptr <= iptr2.m_ptr; }
	friend bool operator>(const CInterfacePtr& iptr1, const CInterfacePtr& iptr2) { return iptr1.m_ptr > iptr2.m_ptr; }
	friend bool operator>=(const CInterfacePtr& iptr1, const CInterfacePtr& iptr2) { return iptr1.m_ptr >= iptr2.m_ptr; }

	template <class INTERFACE2>
	HRESULT QueryInterface(INTERFACE2*& ptr) const { _ASSERTE(m_ptr != NULL); m_ptr->QueryInterface(__uuidof(INTERFACE2), (void**)&ptr); }
	template <class INTERFACE2>
	HRESULT QueryInterface(CInterfacePtr<INTERFACE2>& iptr2) const { _ASSERTE(m_ptr != NULL); _ASSERTE(iptr2.m_ptr == NULL); m_ptr->QueryInterface(__uuidof(INTERFACE2), (void**)&iptr2.m_ptr); }

	bool IsNull(void) const { return (m_ptr == NULL); }
	void Release(void) { Free(); m_ptr = NULL; }
	void Attach(INTERFACE* ptr) { Free(); m_ptr = ptr; }
	INTERFACE* Detach(void) { INTERFACE* ptr = m_ptr; m_ptr = NULL; return ptr; }

	HRESULT CreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter = NULL, DWORD dwClsContext = CLSCTX_ALL) { _ASSERTE(m_ptr == NULL); return CoCreateInstance(rclsid, pUnkOuter, dwClsContext, __uuidof(INTERFACE), (void**)&m_ptr); }
	HRESULT CreateInstance(LPCOLESTR szProgID, LPUNKNOWN pUnkOuter = NULL, DWORD dwClsContext = CLSCTX_ALL);

	template <class INTERFACE2>
	bool IsEqualObject(INTERFACE2* ptr2) const;
	template <class INTERFACE2>
	bool IsEqualObject(const CInterfacePtr<INTERFACE2>& iptr2) const { return IsEqualObject(iptr2.m_ptr); }

private:
	void Assign(INTERFACE* ptr) { if (ptr) ptr->AddRef(); m_ptr = ptr; }
	void Free(void) const { if (m_ptr) m_ptr->Release(); }
	INTERFACE* m_ptr;
};

template <class INTERFACE>
template <class INTERFACE2>
bool CInterfacePtr<INTERFACE>::IsEqualObject(INTERFACE2* ptr2) const
{
	if (m_ptr == NULL && ptr2 == NULL)
		return true;
	if (m_ptr == NULL || ptr2 == NULL)
		return false;
	CInterfacePtr<IUnknown> pUnk1;
	m_ptr->QueryInterface(__uuidof(IUnknown), (void**)&pUnk1);
	_ASSERTE(! pUnk1.IsNull());
	CInterfacePtr<IUnknown> pUnk2;
	ptr2->QueryInterface(__uuidof(IUnknown), (void**)&pUnk2);
	_ASSERTE(! pUnk2.IsNull());
	return (pUnk1 == pUnk2);
}

template <class INTERFACE>
HRESULT CInterfacePtr<INTERFACE>::CreateInstance(LPCOLESTR szProgID, LPUNKNOWN pUnkOuter, DWORD dwClsContext)
{
	CLSID clsid;
	HRESULT hr = CLSIDFromProgID(szProgID, &clsid);
	return (SUCCEEDED(hr) ? CreateInstance(clsid, pUnkOuter, dwClsContext) : hr);
}
