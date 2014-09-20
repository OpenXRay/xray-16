/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Doubly-linked list.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#pragma once

#include "ColHelper.h"

/// Doubly-linked list
template
<
	typename DATA_TYPE,
	class INSTANCE_TRAITS = CObjectTraits<DATA_TYPE>,
	class COMPARE_TRAITS = CCompareTraits<DATA_TYPE>
>
class CList
{
private:
	/// List item.
	class CListItem
	{
		/// Doubly-linked list.
		friend class CList;

	private:
		/// Contains data value.
		DATA_TYPE m_Data;
		/// Pointer to the previous item.
		CListItem* m_pPrev;
		/// Pointer to the next item.
		CListItem* m_pNext;
	};

	/// Pointer to the first list item.
	CListItem* m_pFirst;
	/// Pointer to the last list item.
	CListItem* m_pLast;
	/// List of deleted items. These items can be reused in future allocations.
	CListItem* m_pGarbage;
	/// Number of items in collection.
	int m_nCount;

	/// Initialize list variables.
	void InitList(void);
	/// Throw an exception if pointer to the item is not valid.
	static void ValidateItem(const CListItem* pItem);
	/// Allocate new list item.
	CListItem* AllocListItem(void);

public:
	/// Synonym of list iterator type.
	typedef CListItem* POSITION;

	/// Initialize a list object.
	CList(void);
	/// Makes a copy of a list.
	CList(const CList& rList);
	/// Makes a copy of a list.
	const CList& operator=(const CList& rList);
	/// Destroy a list object and free allocated memory.
	~CList(void);
	/// Tests for the empty list condition (no elements).
	bool IsEmpty(void) const;
	/// Returns the number of elements in this list.
	int GetCount(void) const;
	/// Adds an element to the head of the list (makes a new head).
	POSITION AddToHead(const DATA_TYPE& rData);
	/// Adds an element to the tail of the list (makes a new tail).
	POSITION AddToTail(const DATA_TYPE& rData);
	/// Returns the head element of the list (cannot be empty).
	DATA_TYPE& GetHead(void);
	/// Returns the head element of the list (cannot be empty).
	const DATA_TYPE& GetHead(void) const;
	/// Returns the tail element of the list (cannot be empty).
	DATA_TYPE& GetTail(void);
	/// Returns the tail element of the list (cannot be empty).
	const DATA_TYPE& GetTail(void) const;
	/// Returns the position of the head element of the list.
	POSITION GetHeadPosition(void) const;
	/// Returns the position of the tail element of the list.
	POSITION GetTailPosition(void) const;
	/// Gets the next element for iterating.
	static POSITION GetNextPosition(POSITION pos);
	/// Gets the previous element for iterating.
	static POSITION GetPrevPosition(POSITION pos);
	/// Gets an element at a given position.
	static DATA_TYPE& GetAt(POSITION pos);
	/// Sets an element at a given position.
	static void SetAt(POSITION pos, const DATA_TYPE& rData);
	/// Removes an element from this list, specified by position.
	void DeleteAt(POSITION pos);
	/// Inserts a new element before a given position.
	POSITION InsertBefore(POSITION pos, const DATA_TYPE& rData);
	/// Inserts a new element after a given position.
	POSITION InsertAfter(POSITION pos, const DATA_TYPE& rData);
	/// Removes the element from the head of the list.
	void DeleteHead(void);
	/// Removes the element from the tail of the list.
	void DeleteTail(void);
	/// Removes all the elements from this list.
	void DeleteAll(bool bFree = false);
	/// Gets the position of an element specified by pointer value.
	POSITION Find(const DATA_TYPE& rSearchValue, POSITION startAfter = NULL) const;
	/// Gets the position of an element specified by a zero-based index.
	POSITION GetByIndex(int nIndex) const;
};

template <typename DATA_TYPE, class INSTANCE_TRAITS, class COMPARE_TRAITS>
void CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::InitList(void)
{
	m_pFirst = NULL;
	m_pLast = NULL;
	m_pGarbage = NULL;
	m_nCount = 0;
}

/**
 * @param pItem - validated item.
 */
template <typename DATA_TYPE, class INSTANCE_TRAITS, class COMPARE_TRAITS>
inline void CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::ValidateItem(const CListItem* pItem)
{
	_ASSERTE(pItem);
	if (! pItem)
		RaiseException(STATUS_ARRAY_BOUNDS_EXCEEDED, 0, 0, NULL);
}

template <typename DATA_TYPE, class INSTANCE_TRAITS, class COMPARE_TRAITS>
inline CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::CList(void)
{
	InitList();
}

/**
 * @param rList - object to be copied.
 */
template <typename DATA_TYPE, class INSTANCE_TRAITS, class COMPARE_TRAITS>
CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::CList(const CList& rList)
{
	InitList();

	CListItem* pItem = rList.m_pFirst;
	while (m_nCount < rList.m_nCount)
	{
		AddToTail(pItem->m_Data);
		pItem = pItem->m_pNext;
	}
}

/**
 * @param rList - object to be copied.
 * @return reference to itself.
 */
template <typename DATA_TYPE, class INSTANCE_TRAITS, class COMPARE_TRAITS>
const CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>& CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::operator=(const CList& rList)
{
	if (this != &rList)
	{
		CListItem* pSrcItem = rList.m_pFirst;
		CListItem* pDstItem = m_pFirst;
		if (m_nCount < rList.m_nCount)
		{
			if (m_pFirst == NULL)
			{
				while (m_nCount < rList.m_nCount)
				{
					AddToTail(pSrcItem->m_Data);
					pSrcItem = pSrcItem->m_pNext;
				}
			}
			else
			{
				while (m_nCount < rList.m_nCount)
				{
					InsertBefore(pDstItem, pSrcItem->m_Data);
					pSrcItem = pSrcItem->m_pNext;
				}
			}
		}
		else if (m_nCount > rList.m_nCount)
		{
			while (m_nCount > rList.m_nCount)
				DeleteTail();
		}
		while (pSrcItem != NULL)
		{
			INSTANCE_TRAITS::Assignment(pDstItem->m_Data, pSrcItem->m_Data);
			pSrcItem = pSrcItem->m_pNext;
			pDstItem = pDstItem->m_pNext;
		}
	}
	return *this;
}

template <typename DATA_TYPE, class INSTANCE_TRAITS, class COMPARE_TRAITS>
inline CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::~CList(void)
{
	DeleteAll(true);
}

/**
 * @return newly allocated list item.
 */
template <typename DATA_TYPE, class INSTANCE_TRAITS, class COMPARE_TRAITS>
typename CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::CListItem* CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::AllocListItem(void)
{
	CListItem* pItem;
	if (m_pGarbage)
	{
		pItem = m_pGarbage;
		m_pGarbage = m_pGarbage->m_pNext;
	}
	else
	{
		pItem = (CListItem*)new BYTE[sizeof(CListItem)];
		if (! pItem)
			RaiseException(STATUS_NO_MEMORY, 0, 0, NULL);
	}
	return pItem;
}


/**
 * @param rData - the new element.
 * @return the position of newly inserted value.
 */
template <typename DATA_TYPE, class INSTANCE_TRAITS, class COMPARE_TRAITS>
typename CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::POSITION CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::AddToHead(const DATA_TYPE& rData)
{
	CListItem* pFirst = AllocListItem();
	pFirst->m_pPrev = NULL;
	pFirst->m_pNext = m_pFirst;
	if (m_pFirst)
		m_pFirst->m_pPrev = pFirst;
	else
		m_pLast = pFirst;
	m_pFirst = pFirst;
	INSTANCE_TRAITS::Constructor(pFirst->m_Data);
	INSTANCE_TRAITS::Assignment(pFirst->m_Data, rData);
	++m_nCount;
	return pFirst;
}

/**
 * @param rData - the new element.
 * @return the position of newly inserted value.
 */
template <typename DATA_TYPE, class INSTANCE_TRAITS, class COMPARE_TRAITS>
typename CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::POSITION CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::AddToTail(const DATA_TYPE& rData)
{
	CListItem* pLast = AllocListItem();
	pLast->m_pPrev = m_pLast;
	pLast->m_pNext = NULL;
	if (m_pLast)
		m_pLast->m_pNext = pLast;
	else
		m_pFirst = pLast;
	m_pLast = pLast;
	INSTANCE_TRAITS::Constructor(pLast->m_Data);
	INSTANCE_TRAITS::Assignment(pLast->m_Data, rData);
	++m_nCount;
	return pLast;
}

/**
 * @return a reference to an element of the list.
 */
template <typename DATA_TYPE, class INSTANCE_TRAITS, class COMPARE_TRAITS>
inline DATA_TYPE& CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::GetHead(void)
{
	ValidateItem(m_pFirst);
	return m_pFirst->m_Data;
}

/**
 * @return a reference to an element of the list.
 */
template <typename DATA_TYPE, class INSTANCE_TRAITS, class COMPARE_TRAITS>
inline const DATA_TYPE& CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::GetHead(void) const
{
	ValidateItem(m_pFirst);
	return m_pFirst->m_Data;
}

/**
 * @return a reference to an element of the list.
 */
template <typename DATA_TYPE, class INSTANCE_TRAITS, class COMPARE_TRAITS>
inline DATA_TYPE& CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::GetTail(void)
{
	ValidateItem(m_pLast);
	return m_pLast->m_Data;
}

/**
 * @return a reference to an element of the list.
 */
template <typename DATA_TYPE, class INSTANCE_TRAITS, class COMPARE_TRAITS>
inline const DATA_TYPE& CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::GetTail(void) const
{
	ValidateItem(m_pLast);
	return m_pLast->m_Data;
}

/**
 * @return a @a POSITION value that can be used for iteration or object pointer retrieval; NULL if the list is empty.
 */
template <typename DATA_TYPE, class INSTANCE_TRAITS, class COMPARE_TRAITS>
inline typename CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::POSITION CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::GetHeadPosition(void) const
{
	return m_pFirst;
}

/**
 * @return a @a POSITION value that can be used for iteration or object pointer retrieval; NULL if the list is empty.
 */
template <typename DATA_TYPE, class INSTANCE_TRAITS, class COMPARE_TRAITS>
inline typename CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::POSITION CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::GetTailPosition(void) const
{
	return m_pLast;
}

/**
 * @param pos - a @a POSITION value returned by a previous GetPrevPosition(), GetTailPosition(), or other member function call.
 * @return the position of next element is the list or NULL.
 */
template <typename DATA_TYPE, class INSTANCE_TRAITS, class COMPARE_TRAITS>
inline typename CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::POSITION CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::GetNextPosition(typename CList::POSITION pos)
{
	ValidateItem(pos);
	return pos->m_pNext;
}

/**
 * @param pos - a @a POSITION value returned by a previous GetPrevPosition(), GetTailPosition(), or other member function call.
 * @return the position of next element is the list or NULL.
 */
template <typename DATA_TYPE, class INSTANCE_TRAITS, class COMPARE_TRAITS>
inline typename CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::POSITION CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::GetPrevPosition(typename CList::POSITION pos)
{
	ValidateItem(pos);
	return pos->m_pPrev;
}

/**
 * @return true if this list is empty; otherwise false.
 */
template <typename DATA_TYPE, class INSTANCE_TRAITS, class COMPARE_TRAITS>
inline bool CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::IsEmpty(void) const
{
	return (m_pFirst == NULL);
}

/**
 * @return an integer value containing the element count.
 */
template <typename DATA_TYPE, class INSTANCE_TRAITS, class COMPARE_TRAITS>
inline int CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::GetCount(void) const
{
	return m_nCount;
}

/**
 * @param pos - a @a POSITION value returned by a previous GetPrevPosition(), GetTailPosition(), or other member function call.
 * @return a reference to an element of the list.
 */
template <typename DATA_TYPE, class INSTANCE_TRAITS, class COMPARE_TRAITS>
inline DATA_TYPE& CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::GetAt(typename CList::POSITION pos)
{
	ValidateItem(pos);
	return pos->m_Data;
}

/**
 * @param pos - the @a POSITION of the element to be set.
 * @param rData - the element to be added to the list.
 */
template <typename DATA_TYPE, class INSTANCE_TRAITS, class COMPARE_TRAITS>
inline void CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::SetAt(typename CList::POSITION pos, const DATA_TYPE& rData)
{
	ValidateItem(pos);
	INSTANCE_TRAITS::Assignment(pos->m_Data, rData);
}

/**
 * @param pos - the position of the element to be removed from the list.
 */
template <typename DATA_TYPE, class INSTANCE_TRAITS, class COMPARE_TRAITS>
void CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::DeleteAt(typename CList::POSITION pos)
{
	ValidateItem(pos);
	CListItem* pPrev = pos->m_pPrev;
	CListItem* pNext = pos->m_pNext;
	if (pPrev)
		pPrev->m_pNext = pNext;
	else
		m_pFirst = pNext;
	if (pNext)
		pNext->m_pPrev = pPrev;
	else
		m_pLast = pPrev;
	INSTANCE_TRAITS::Destructor(pos->m_Data);
	pos->m_pNext = m_pGarbage;
	m_pGarbage = pos;
	--m_nCount;
}

/**
 * @param pos - a @a POSITION value returned by a previous GetPrevPosition(), GetTailPosition(), or other member function call.
 * @param rData - the element to be added to this list.
 * @return the position of newly inserted value.
 */
template <typename DATA_TYPE, class INSTANCE_TRAITS, class COMPARE_TRAITS>
typename CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::POSITION CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::InsertBefore(typename CList::POSITION pos, const DATA_TYPE& rData)
{
	ValidateItem(pos);
	CListItem* pItem = AllocListItem();
	CListItem* pPrev = pos->m_pPrev;
	pItem->m_pPrev = pPrev;
	pItem->m_pNext = pos;
	if (pPrev)
		pPrev->m_pNext = pItem;
	else
		m_pFirst = pItem;
	pos->m_pPrev = pItem;
	INSTANCE_TRAITS::Constructor(pItem->m_Data);
	INSTANCE_TRAITS::Assignment(pItem->m_Data, rData);
	++m_nCount;
	return pItem;
}

/**
 * @param pos - a @a POSITION value returned by a previous GetPrevPosition(), GetTailPosition(), or other member function call.
 * @param rData - the element to be added to this list.
 * @return the position of newly inserted value.
 */
template <typename DATA_TYPE, class INSTANCE_TRAITS, class COMPARE_TRAITS>
typename CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::POSITION CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::InsertAfter(typename CList::POSITION pos, const DATA_TYPE& rData)
{
	ValidateItem(pos);
	CListItem* pItem = AllocListItem();
	CListItem* pNext = pos->m_pNext;
	pItem->m_pPrev = pos;
	pItem->m_pNext = pNext;
	if (pNext)
		pNext->m_pPrev = pItem;
	else
		m_pLast = pItem;
	pos->m_pNext = pItem;
	INSTANCE_TRAITS::Constructor(pItem->m_Data);
	INSTANCE_TRAITS::Assignment(pItem->m_Data, rData);
	++m_nCount;
	return pItem;
}

template <typename DATA_TYPE, class INSTANCE_TRAITS, class COMPARE_TRAITS>
void CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::DeleteHead(void)
{
	ValidateItem(m_pFirst);
	INSTANCE_TRAITS::Destructor(m_pFirst->m_Data);
	CListItem* pNext = m_pFirst->m_pNext;
	m_pFirst->m_pNext = m_pGarbage;
	m_pGarbage = m_pFirst;
	m_pFirst = pNext;
	if (pNext)
		pNext->m_pPrev = NULL;
	else
		m_pLast = NULL;
	--m_nCount;
}

template <typename DATA_TYPE, class INSTANCE_TRAITS, class COMPARE_TRAITS>
void CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::DeleteTail(void)
{
	ValidateItem(m_pLast);
	INSTANCE_TRAITS::Destructor(m_pLast->m_Data);
	CListItem* pPrev = m_pLast->m_pPrev;
	m_pLast->m_pNext = m_pGarbage;
	m_pGarbage = m_pLast;
	m_pLast = pPrev;
	if (pPrev)
		pPrev->m_pNext = NULL;
	else
		m_pFirst = NULL;
	--m_nCount;
}

/**
 * @param bFree - pass true of release array memory.
 */
template <typename DATA_TYPE, class INSTANCE_TRAITS, class COMPARE_TRAITS>
void CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::DeleteAll(bool bFree)
{
	if (bFree)
	{
		CListItem* pNext;
		while (m_pGarbage)
		{
			pNext = m_pGarbage->m_pNext;
			delete[] (PBYTE)m_pGarbage;
			m_pGarbage = pNext;
		}
		while (m_pFirst)
		{
			pNext = m_pFirst->m_pNext;
			INSTANCE_TRAITS::Destructor(m_pFirst->m_Data);
			delete[] (PBYTE)m_pFirst;
			m_pFirst = pNext;
		}
	}
	else
	{
		while (m_pFirst)
		{
			CListItem* pNext = m_pFirst->m_pNext;
			INSTANCE_TRAITS::Destructor(m_pFirst->m_Data);
			m_pFirst->m_pNext = m_pGarbage;
			m_pGarbage = m_pFirst;
			m_pFirst = pNext;
		}
	}
	m_pLast = NULL;
	m_nCount = 0;
}

/**
 * @param rSearchValue - the value to be found in the list.
 * @param startAfter - the start position for the search.
 * @return a @a POSITION value that can be used for iteration or object pointer retrieval; NULL if the object is not found.
 */
template <typename DATA_TYPE, class INSTANCE_TRAITS, class COMPARE_TRAITS>
typename CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::POSITION CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::Find(const DATA_TYPE& rSearchValue, typename CList::POSITION startAfter) const
{
	if (startAfter == NULL)
		startAfter = m_pFirst;
	while (startAfter != NULL && ! COMPARE_TRAITS::Compare(startAfter->m_Data, rSearchValue))
		startAfter = startAfter->m_pNext;
	return startAfter;
}

/**
 * @param nIndex - the zero-based index of the list element to be found.
 * @return a @a POSITION value that can be used for iteration or object pointer retrieval; NULL if nIndex is negative or too large.
 */
template <typename DATA_TYPE, class INSTANCE_TRAITS, class COMPARE_TRAITS>
typename CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::POSITION CList<DATA_TYPE, INSTANCE_TRAITS, COMPARE_TRAITS>::GetByIndex(int nIndex) const
{
	if (nIndex < 0)
		return NULL;
	CListItem* pItem = m_pFirst;
	while (nIndex-- && pItem)
		pItem = pItem->m_pNext;
	return pItem;
}
