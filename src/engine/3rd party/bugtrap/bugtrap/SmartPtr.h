/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Smart pointer class.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#pragma once

struct CPtrTraits
{
	template <typename TYPE>
	static void Destroy(TYPE* ptr) { delete ptr; }
};

struct CArrayTraits
{
	template <typename TYPE>
	static void Destroy(TYPE* ptr) { delete[] ptr; }
};

template <typename TYPE, class TRAITS>
class CPtrLinkEngine
{
protected:
	TYPE* GetPtr(void) const { return m_ptr; }
	void Assign(TYPE* ptr);
	void Assign(const CPtrLinkEngine& sptr);
	void Release(void);

private:
	TYPE* m_ptr;
	mutable const CPtrLinkEngine* m_pPrev;
	mutable const CPtrLinkEngine* m_pNext;
};

template <typename TYPE, class TRAITS>
void CPtrLinkEngine<TYPE, TRAITS>::Assign(TYPE* ptr)
{
	m_ptr = ptr;
	m_pPrev = this;
	m_pNext = this;
}

template <typename TYPE, class TRAITS>
void CPtrLinkEngine<TYPE, TRAITS>::Assign(const CPtrLinkEngine& sptr)
{
	m_ptr = sptr.m_ptr;
	m_pPrev = sptr.m_pPrev;
	m_pNext = &sptr;
	sptr.m_pPrev = this;
	m_pPrev->m_pNext = this;
}

template <typename TYPE, class TRAITS>
void CPtrLinkEngine<TYPE, TRAITS>::Release(void)
{
	if (m_pNext == this)
	{
		TRAITS::Destroy(m_ptr);
	}
	else
	{
		m_pPrev->m_pNext = m_pNext;
		m_pNext->m_pPrev = m_pPrev;
	}
}

template <typename TYPE, class TRAITS>
class CPtrRefEngine
{
protected:
	TYPE* GetPtr(void) const { _ASSERTE(m_pData != NULL); return m_pData->m_ptr; }
	void Assign(TYPE* ptr);
	void Assign(const CPtrRefEngine& sptr);
	void Release(void);

private:
	struct CPtrData
	{
		CPtrData(void);
		CPtrData(TYPE* ptr);
		unsigned m_uNumRefs;
		TYPE* m_ptr;
	};
	static CPtrData m_EmptyData;
	CPtrData* m_pData;
};

template <typename TYPE, class TRAITS>
inline CPtrRefEngine<TYPE, TRAITS>::CPtrData::CPtrData(void)
{
	m_ptr = NULL;
	m_uNumRefs = 0;
}

template <typename TYPE, class TRAITS>
inline CPtrRefEngine<TYPE, TRAITS>::CPtrData::CPtrData(TYPE* ptr)
{
	m_ptr = ptr;
	m_uNumRefs = 1;
}

template <typename TYPE, class TRAITS>
typename CPtrRefEngine<TYPE, TRAITS>::CPtrData CPtrRefEngine<TYPE, TRAITS>::m_EmptyData;

template <typename TYPE, class TRAITS>
void CPtrRefEngine<TYPE, TRAITS>::Assign(TYPE* ptr)
{
	m_pData = ptr != NULL ? new CPtrData(ptr) : &m_EmptyData;
}

template <typename TYPE, class TRAITS>
void CPtrRefEngine<TYPE, TRAITS>::Assign(const CPtrRefEngine& sptr)
{
	_ASSERTE(sptr.m_pData != NULL);
	m_pData = sptr.m_pData;
	if (m_pData != &m_EmptyData)
		++m_pData->m_uNumRefs;
}

template <typename TYPE, class TRAITS>
void CPtrRefEngine<TYPE, TRAITS>::Release(void)
{
	_ASSERTE(m_pData != NULL);
	if (m_pData != &m_EmptyData)
	{
		if (m_pData->m_uNumRefs == 1)
		{
			_ASSERTE(m_pData->m_ptr != NULL);
			TRAITS::Destroy(m_pData->m_ptr);
			delete m_pData;
		}
		else
			--m_pData->m_uNumRefs;
	}
}

template <typename TYPE, class TRAITS, class ENGINE>
class CBaseSmartPtr : protected ENGINE
{
public:
	CBaseSmartPtr(void) { Assign(NULL); }
	explicit CBaseSmartPtr(TYPE* ptr) { Assign(ptr); }
	CBaseSmartPtr(const CBaseSmartPtr& sptr) { Assign(sptr); }
	~CBaseSmartPtr(void) { Release(); }
	const CBaseSmartPtr& operator=(TYPE* ptr);
	const CBaseSmartPtr& operator=(const CBaseSmartPtr& sptr);

	TYPE& operator*(void) const { _ASSERTE(GetPtr() != NULL); return *GetPtr(); }
	TYPE* operator->(void) const { _ASSERTE(GetPtr() != NULL); return GetPtr(); }
	bool operator!(void) const { return (GetPtr() == NULL); }
	operator TYPE*(void) const { return GetPtr(); }

	friend bool operator==(const CBaseSmartPtr& sptr1, const CBaseSmartPtr& sptr2) { return sptr1.GetPtr() == sptr2.GetPtr(); }
	friend bool operator!=(const CBaseSmartPtr& sptr1, const CBaseSmartPtr& sptr2) { return sptr1.GetPtr() != sptr2.GetPtr(); }
	friend bool operator<(const CBaseSmartPtr& sptr1, const CBaseSmartPtr& sptr2) { return sptr1.GetPtr() < sptr2.GetPtr(); }
	friend bool operator<=(const CBaseSmartPtr& sptr1, const CBaseSmartPtr& sptr2) { return sptr1.GetPtr() <= sptr2.GetPtr(); }
	friend bool operator>(const CBaseSmartPtr& sptr1, const CBaseSmartPtr& sptr2) { return sptr1.GetPtr() > sptr2.GetPtr(); }
	friend bool operator>=(const CBaseSmartPtr& sptr1, const CBaseSmartPtr& sptr2) { return sptr1.GetPtr() >= sptr2.GetPtr(); }
};

template <typename TYPE, class TRAITS, class ENGINE>
const CBaseSmartPtr<TYPE, TRAITS, ENGINE>& CBaseSmartPtr<TYPE, TRAITS, ENGINE>::operator=(TYPE* ptr)
{
	Release();
	Assign(ptr);
	return *this;
}

template <typename TYPE, class TRAITS, class ENGINE>
const CBaseSmartPtr<TYPE, TRAITS, ENGINE>& CBaseSmartPtr<TYPE, TRAITS, ENGINE>::operator=(const CBaseSmartPtr& sptr)
{
	if (this != &sptr)
	{
		Release();
		Assign(sptr);
	}
	return *this;
}

#define CBaseClass CBaseSmartPtr<TYPE, CPtrTraits, ENGINE>

template <typename TYPE, class ENGINE = CPtrRefEngine<TYPE, CPtrTraits> >
class CSmartPtr : public CBaseClass
{
public:
	CSmartPtr(void) : CBaseClass() { }
	explicit CSmartPtr(TYPE* ptr) : CBaseClass(ptr) { }
	CSmartPtr(const CSmartPtr& sptr) : CBaseClass(sptr) { }
	const CSmartPtr& operator=(TYPE* ptr) { CBaseClass::operator=(ptr); return *this; }
	const CSmartPtr& operator=(const CSmartPtr& sptr) { CBaseClass::operator=(sptr); return *this; }
};

#undef CBaseClass

#define CBaseClass CBaseSmartPtr<TYPE, CArrayTraits, ENGINE>

template <typename TYPE, class ENGINE = CPtrRefEngine<TYPE, CArrayTraits> >
class CSmartArray : public CBaseClass
{
public:
	CSmartArray(void) : CBaseClass() { }
	explicit CSmartArray(TYPE* ptr) : CBaseClass(ptr) { }
	CSmartArray(const CSmartArray& sptr) : CBaseClass(sptr) { }
	const CSmartArray& operator=(TYPE* ptr) { CBaseClass::operator=(ptr); return *this; }
	const CSmartArray& operator=(const CSmartArray& sptr) { CBaseClass::operator=(sptr); return *this; }
	TYPE& operator[](INT_PTR index) const { _ASSERTE(GetPtr() != NULL); _ASSERTE(index >= 0); return GetPtr()[index]; }
};

#undef CBaseClass
