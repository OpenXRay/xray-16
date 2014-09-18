/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Hash table.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#pragma once

#include "ColHelper.h"

/// Hash table.
template
<
	typename KEY_TYPE,
	typename DATA_TYPE,
	class KEY_INSTANCE_TRAITS = CObjectTraits<KEY_TYPE>,
	class DATA_INSTANCE_TRAITS = CObjectTraits<DATA_TYPE>,
	class COMPARE_TRAITS = CCompareTraits<KEY_TYPE>,
	class HASH_TRAITS = CHashTraits<KEY_TYPE>
>
class CHash
{
private:
	/// Hash item.
	class CHashItem
	{
		/// Hash table.
		friend class CHash;

	private:
		/// Contains key value.
		KEY_TYPE m_Key;
		/// Contains data value.
		DATA_TYPE m_Data;
		/// Pointer to the next hash item.
		CHashItem* m_pNext;
	};

	/// Hash iterator.
	class CHashIterator
	{
		/// Hash table.
		friend class CHash;

	private:
		/// Pointer to current hash item.
		CHashItem* m_pItem;
		/// Index in the internal array containing current hash items.
		int m_nIndex;

	public:
		/// Return true if iterator points to valid value.
		operator bool(void) const { return (m_pItem != NULL); }
		/// Return false if iterator points to valid value.
		bool operator!(void) const { return (m_pItem == NULL); }
	};

	/// Dynamic array of hash items arranged as list.
	CHashItem** m_pHash;
	/// List of deleted hash items. These items can be reused in future allocations.
	CHashItem* m_pGarbage;
	/// Size of dynamic array.
	int m_nHashSize;
	/// Number of items in collection.
	int m_nCount;

public:
	/// Synonym of hash iterator type.
	typedef CHashIterator POSITION;

	enum
	{
		/// Default hash table size.
		DEFAULT_SIZE = 11
	};

	/// Initialize a hash table object with specified size.
	explicit CHash(int nHashSize = DEFAULT_SIZE);
	/// Makes a copy of a hash table.
	CHash(const CHash& rHash);
	/// Makes a copy of a hash table.
	const CHash& operator=(const CHash& rHash);
	/// Destroy a hash table object and free allocated memory.
	~CHash(void);
	/// Return the size of  dictionary.
	int GetSize(void) const;
	/// Return number of items in collection.
	int GetCount(void) const;
	/// Tests for the empty-hash condition (no elements).
	bool IsEmpty(void) const;
	/// Search for data mapped to a given key. Returns true if the key was found.
	bool Lookup(const KEY_TYPE& rKey, DATA_TYPE& rData);
	/// Search for data mapped to a given key. Returns pointer to founded data or NULL if a matching key is not found.
	DATA_TYPE* Lookup(const KEY_TYPE& rKey);
	/// Search for data mapped to a given key. Returns pointer to founded data or NULL if a matching key is not found.
	const DATA_TYPE* Lookup(const KEY_TYPE& rKey) const;
	/// Insert an element into the dictionary; replace an existing element if a matching key is found.
	void SetAt(const KEY_TYPE& rKey, const DATA_TYPE& rData);
	/// Insert an element into the dictionary - operator substitution for SetAt.
	DATA_TYPE& GetAt(const KEY_TYPE& rKey);
	/// Insert an element into the dictionary - operator substitution for SetAt.
	DATA_TYPE& operator[](const KEY_TYPE& rKey);
	/// Remove an element specified by a key.
	void Delete(const typename KEY_TYPE& rKey);
	/// Remove an element specified by a position.
	void DeleteAt(const typename POSITION& pos);
	/// Remove all the elements from this dictionary.
	void DeleteAll(bool bFree = false);
	/// Returns the position of the first element.
	POSITION GetStartPosition(void) const;
	/// Gets the next element for iterating.
	POSITION GetNextPosition(const typename POSITION& pos) const;
	/// Get the key at a given position.
	static const KEY_TYPE& GetKeyAt(const typename POSITION& pos);
	/// Get the data at a given position.
	static DATA_TYPE& GetDataAt(const typename POSITION& pos);

private:
	/// Look for hash item by the specified key.
	CHashItem* InternalLookup(const typename KEY_TYPE& rKey, int& rIndex) const;
	/// Delete hash item from the hash table.
	void InternalDelete(CHashItem* pDelItem, int nIndex);
	/// Initialize hash table.
	void InitHashTable(int nHashSize);
	/// Throw an exception if position is not valid.
	static void ValidatePosition(const typename POSITION& pos);
};

/**
 * @param nHashSize - hash table size.
 */
template <typename KEY_TYPE, typename DATA_TYPE, class KEY_INSTANCE_TRAITS, class DATA_INSTANCE_TRAITS, class COMPARE_TRAITS, class HASH_TRAITS>
void CHash<KEY_TYPE, DATA_TYPE, KEY_INSTANCE_TRAITS, DATA_INSTANCE_TRAITS, COMPARE_TRAITS, HASH_TRAITS>::InitHashTable(int nHashSize)
{
	m_pGarbage = NULL;
	m_nHashSize = nHashSize;
	m_nCount = 0;
	m_pHash = new CHashItem*[nHashSize];
	if (! m_pHash)
		RaiseException(STATUS_NO_MEMORY, 0, 0, NULL);
}

/**
 * @param nHashSize - hash table size.
 */
template <typename KEY_TYPE, typename DATA_TYPE, class KEY_INSTANCE_TRAITS, class DATA_INSTANCE_TRAITS, class COMPARE_TRAITS, class HASH_TRAITS>
CHash<KEY_TYPE, DATA_TYPE, KEY_INSTANCE_TRAITS, DATA_INSTANCE_TRAITS, COMPARE_TRAITS, HASH_TRAITS>::CHash(int nHashSize)
{
	InitHashTable(nHashSize);
	for (int nIndex = 0; nIndex < nHashSize; ++nIndex)
		m_pHash[nIndex] = NULL;
}

/**
 * @param pos - validated position.
 */
template <typename KEY_TYPE, typename DATA_TYPE, class KEY_INSTANCE_TRAITS, class DATA_INSTANCE_TRAITS, class COMPARE_TRAITS, class HASH_TRAITS>
inline void CHash<KEY_TYPE, DATA_TYPE, KEY_INSTANCE_TRAITS, DATA_INSTANCE_TRAITS, COMPARE_TRAITS, HASH_TRAITS>::ValidatePosition(const typename CHash::POSITION& pos)
{
	_ASSERTE(pos);
	if (! pos)
		RaiseException(STATUS_ARRAY_BOUNDS_EXCEEDED, 0, 0, NULL);
}

/**
 * @param rHash - object to be copied.
 */
template <typename KEY_TYPE, typename DATA_TYPE, class KEY_INSTANCE_TRAITS, class DATA_INSTANCE_TRAITS, class COMPARE_TRAITS, class HASH_TRAITS>
CHash<KEY_TYPE, DATA_TYPE, KEY_INSTANCE_TRAITS, DATA_INSTANCE_TRAITS, COMPARE_TRAITS, HASH_TRAITS>::CHash(const CHash& rHash)
{
	InitHashTable(rHash.m_nHashSize);
	for (int nIndex = 0; nIndex < m_nHashSize; ++nIndex)
	{
		m_pHash[nIndex] = NULL;
		CHashItem* pItem = rHash.m_pHash[nIndex];
		while (pItem)
		{
			SetAt(pItem->m_Key, pItem->m_Data);
			pItem = pItem->m_pNext;
		}
	}
}

/**
 * @param rHash - object to be copied.
 * @return reference to itself.
 */
template <typename KEY_TYPE, typename DATA_TYPE, class KEY_INSTANCE_TRAITS, class DATA_INSTANCE_TRAITS, class COMPARE_TRAITS, class HASH_TRAITS>
const CHash<KEY_TYPE, DATA_TYPE, KEY_INSTANCE_TRAITS, DATA_INSTANCE_TRAITS, COMPARE_TRAITS, HASH_TRAITS>& CHash<KEY_TYPE, DATA_TYPE, KEY_INSTANCE_TRAITS, DATA_INSTANCE_TRAITS, COMPARE_TRAITS, HASH_TRAITS>::operator=(const CHash& rHash)
{
	if (this != &rHash)
	{
		DeleteAll();
		for (int nIndex = 0; nIndex < rHash.m_nHashSize; ++nIndex)
		{
			CHashItem* pItem = rHash.m_pHash[nIndex];
			while (pItem)
			{
				SetAt(pItem->m_Key, pItem->m_Data);
				pItem = pItem->m_pNext;
			}
		}
	}
	return *this;
}

template <typename KEY_TYPE, typename DATA_TYPE, class KEY_INSTANCE_TRAITS, class DATA_INSTANCE_TRAITS, class COMPARE_TRAITS, class HASH_TRAITS>
inline CHash<KEY_TYPE, DATA_TYPE, KEY_INSTANCE_TRAITS, DATA_INSTANCE_TRAITS, COMPARE_TRAITS, HASH_TRAITS>::~CHash(void)
{
	DeleteAll(true);
	delete[] m_pHash;
}

/**
 * @return size of hash table.
 */
template <typename KEY_TYPE, typename DATA_TYPE, class KEY_INSTANCE_TRAITS, class DATA_INSTANCE_TRAITS, class COMPARE_TRAITS, class HASH_TRAITS>
inline int CHash<KEY_TYPE, DATA_TYPE, KEY_INSTANCE_TRAITS, DATA_INSTANCE_TRAITS, COMPARE_TRAITS, HASH_TRAITS>::GetSize(void) const
{
	return m_nHashSize;
}

/**
 * @return the number of elements.
 */
template <typename KEY_TYPE, typename DATA_TYPE, class KEY_INSTANCE_TRAITS, class DATA_INSTANCE_TRAITS, class COMPARE_TRAITS, class HASH_TRAITS>
inline int CHash<KEY_TYPE, DATA_TYPE, KEY_INSTANCE_TRAITS, DATA_INSTANCE_TRAITS, COMPARE_TRAITS, HASH_TRAITS>::GetCount(void) const
{
	return m_nCount;
}

/**
 * @return true if this hash contains no elements; otherwise false.
 */
template <typename KEY_TYPE, typename DATA_TYPE, class KEY_INSTANCE_TRAITS, class DATA_INSTANCE_TRAITS, class COMPARE_TRAITS, class HASH_TRAITS>
inline bool CHash<KEY_TYPE, DATA_TYPE, KEY_INSTANCE_TRAITS, DATA_INSTANCE_TRAITS, COMPARE_TRAITS, HASH_TRAITS>::IsEmpty(void) const
{
	return (m_nCount == 0);
}

/**
 * @param rKey - specifies the key that identifies the element to be looked up.
 * @param rIndex - index in array of obtained item.
 * @return pointer to obtained hash item or NULL.
 */
template <typename KEY_TYPE, typename DATA_TYPE, class KEY_INSTANCE_TRAITS, class DATA_INSTANCE_TRAITS, class COMPARE_TRAITS, class HASH_TRAITS>
typename CHash<KEY_TYPE, DATA_TYPE, KEY_INSTANCE_TRAITS, DATA_INSTANCE_TRAITS, COMPARE_TRAITS, HASH_TRAITS>::CHashItem* CHash<KEY_TYPE, DATA_TYPE, KEY_INSTANCE_TRAITS, DATA_INSTANCE_TRAITS, COMPARE_TRAITS, HASH_TRAITS>::InternalLookup(const typename KEY_TYPE& rKey, int& rIndex) const
{
	rIndex = HASH_TRAITS::HashKey(rKey) % m_nHashSize;
	CHashItem* pItem = m_pHash[rIndex];
	while (pItem)
	{
		if (COMPARE_TRAITS::Compare(pItem->m_Key, rKey))
			return pItem;
		pItem = pItem->m_pNext;
	}
	return NULL;
}

/**
 * @param rKey - specifies the key that identifies the element to be looked up.
 * @param rData - data variable that is being set to found value.
 * @return true if the key was found.
 */
template <typename KEY_TYPE, typename DATA_TYPE, class KEY_INSTANCE_TRAITS, class DATA_INSTANCE_TRAITS, class COMPARE_TRAITS, class HASH_TRAITS>
inline bool CHash<KEY_TYPE, DATA_TYPE, KEY_INSTANCE_TRAITS, DATA_INSTANCE_TRAITS, COMPARE_TRAITS, HASH_TRAITS>::Lookup(const KEY_TYPE& rKey, DATA_TYPE& rData)
{
	int nIndex;
	CHashItem* pItem = InternalLookup(rKey, nIndex);
	if (pItem)
	{
		rData = pItem->m_Data;
		return true;
	}
	return false;
}

/**
 * @param rKey - specifies the key that identifies the element to be looked up.
 * @return pointer to obtained hash item or NULL.
 */
template <typename KEY_TYPE, typename DATA_TYPE, class KEY_INSTANCE_TRAITS, class DATA_INSTANCE_TRAITS, class COMPARE_TRAITS, class HASH_TRAITS>
inline DATA_TYPE* CHash<KEY_TYPE, DATA_TYPE, KEY_INSTANCE_TRAITS, DATA_INSTANCE_TRAITS, COMPARE_TRAITS, HASH_TRAITS>::Lookup(const KEY_TYPE& rKey)
{
	int nIndex;
	CHashItem* pItem = InternalLookup(rKey, nIndex);
	return (pItem ? &pItem->m_Data : NULL);
}

/**
 * @param rKey - specifies the key that identifies the element to be looked up.
 * @return pointer to obtained hash item or NULL.
 */
template <typename KEY_TYPE, typename DATA_TYPE, class KEY_INSTANCE_TRAITS, class DATA_INSTANCE_TRAITS, class COMPARE_TRAITS, class HASH_TRAITS>
inline const DATA_TYPE* CHash<KEY_TYPE, DATA_TYPE, KEY_INSTANCE_TRAITS, DATA_INSTANCE_TRAITS, COMPARE_TRAITS, HASH_TRAITS>::Lookup(const KEY_TYPE& rKey) const
{
	int nIndex;
	CHashItem* pItem = InternalLookup(rKey, nIndex);
	return (pItem ? &pItem->m_Data : NULL);
}

/**
 * @param rKey - specifies the key of the new element.
 * @param rData - specifies the value of the new element.
 */
template <typename KEY_TYPE, typename DATA_TYPE, class KEY_INSTANCE_TRAITS, class DATA_INSTANCE_TRAITS, class COMPARE_TRAITS, class HASH_TRAITS>
inline void CHash<KEY_TYPE, DATA_TYPE, KEY_INSTANCE_TRAITS, DATA_INSTANCE_TRAITS, COMPARE_TRAITS, HASH_TRAITS>::SetAt(const KEY_TYPE& rKey, const DATA_TYPE& rData)
{
	DATA_INSTANCE_TRAITS::Assignment(GetAt(rKey), rData);
}

/**
 * @param rKey - the key used to retrieve the value from the hash.
 * @return reference to the item.
 */
template <typename KEY_TYPE, typename DATA_TYPE, class KEY_INSTANCE_TRAITS, class DATA_INSTANCE_TRAITS, class COMPARE_TRAITS, class HASH_TRAITS>
DATA_TYPE& CHash<KEY_TYPE, DATA_TYPE, KEY_INSTANCE_TRAITS, DATA_INSTANCE_TRAITS, COMPARE_TRAITS, HASH_TRAITS>::GetAt(const KEY_TYPE& rKey)
{
	int nIndex;
	CHashItem* pItem = InternalLookup(rKey, nIndex);
	if (pItem == NULL)
	{
		if (m_pGarbage)
		{
			pItem = m_pGarbage;
			m_pGarbage = m_pGarbage->m_pNext;
		}
		else
		{
			pItem = (CHashItem*)new BYTE[sizeof(CHashItem)];
			if (! pItem)
				RaiseException(STATUS_NO_MEMORY, 0, 0, NULL);
		}
		KEY_INSTANCE_TRAITS::Constructor(pItem->m_Key);
		KEY_INSTANCE_TRAITS::Assignment(pItem->m_Key, rKey);
		DATA_INSTANCE_TRAITS::Constructor(pItem->m_Data);
		pItem->m_pNext = m_pHash[nIndex];
		m_pHash[nIndex] = pItem;
		++m_nCount;
	}
	return pItem->m_Data;
}

/**
 * @param rKey - the key used to retrieve the value from the hash.
 * @return reference to the item.
 */
template <typename KEY_TYPE, typename DATA_TYPE, class KEY_INSTANCE_TRAITS, class DATA_INSTANCE_TRAITS, class COMPARE_TRAITS, class HASH_TRAITS>
inline DATA_TYPE& CHash<KEY_TYPE, DATA_TYPE, KEY_INSTANCE_TRAITS, DATA_INSTANCE_TRAITS, COMPARE_TRAITS, HASH_TRAITS>::operator[](const KEY_TYPE& rKey)
{
	return GetAt(rKey);
}

/**
 * @param pDelItem - pointer to deleted hash item.
 * @param nIndex - index of array there deleted item can be found.
 */
template <typename KEY_TYPE, typename DATA_TYPE, class KEY_INSTANCE_TRAITS, class DATA_INSTANCE_TRAITS, class COMPARE_TRAITS, class HASH_TRAITS>
void CHash<KEY_TYPE, DATA_TYPE, KEY_INSTANCE_TRAITS, DATA_INSTANCE_TRAITS, COMPARE_TRAITS, HASH_TRAITS>::InternalDelete(typename CHash::CHashItem* pDelItem, int nIndex)
{
	CHashItem* pPrev = NULL;
	CHashItem* pItem = m_pHash[nIndex];
	while (pItem)
	{
		if (pItem == pDelItem)
		{
			if (pPrev)
				pPrev->m_pNext = pItem->m_pNext;
			else
				m_pHash[nIndex] = pItem->m_pNext;
			KEY_INSTANCE_TRAITS::Destructor(pItem->m_Key);
			DATA_INSTANCE_TRAITS::Destructor(pItem->m_Data);
			pItem->m_pNext = m_pGarbage;
			m_pGarbage = pItem;
			--m_nCount;
			break;
		}
		else
		{
			pPrev = pItem;
			pItem = pItem->m_pNext;
		}
	}
}

/**
 * @param rKey - key for the element to be removed.
 */
template <typename KEY_TYPE, typename DATA_TYPE, class KEY_INSTANCE_TRAITS, class DATA_INSTANCE_TRAITS, class COMPARE_TRAITS, class HASH_TRAITS>
inline void CHash<KEY_TYPE, DATA_TYPE, KEY_INSTANCE_TRAITS, DATA_INSTANCE_TRAITS, COMPARE_TRAITS, HASH_TRAITS>::Delete(const typename KEY_TYPE& rKey)
{
	int nIndex;
	CHashItem* pItem = InternalLookup(rKey, nIndex);
	if (! pItem)
		RaiseException(STATUS_ARRAY_BOUNDS_EXCEEDED, 0, 0, NULL);
	InternalDelete(pItem, nIndex);
}

/**
 * @param bFree - pass true of release array memory.
 */
template <typename KEY_TYPE, typename DATA_TYPE, class KEY_INSTANCE_TRAITS, class DATA_INSTANCE_TRAITS, class COMPARE_TRAITS, class HASH_TRAITS>
void CHash<KEY_TYPE, DATA_TYPE, KEY_INSTANCE_TRAITS, DATA_INSTANCE_TRAITS, COMPARE_TRAITS, HASH_TRAITS>::DeleteAll(bool bFree)
{
	if (bFree)
	{
		CHashItem* pNext;
		while (m_pGarbage)
		{
			pNext = m_pGarbage->m_pNext;
			delete[] (PBYTE)m_pGarbage;
			m_pGarbage = pNext;
		}
		for (int nIndex = 0; nIndex < m_nHashSize; ++nIndex)
		{
			CHashItem* pItem = m_pHash[nIndex];
			while (pItem)
			{
				pNext = pItem->m_pNext;
				KEY_INSTANCE_TRAITS::Destructor(pItem->m_Key);
				DATA_INSTANCE_TRAITS::Destructor(pItem->m_Data);
				delete[] (PBYTE)pItem;
				pItem = pNext;
			}
			m_pHash[nIndex] = NULL;
		}
	}
	else
	{
		for (int nIndex = 0; nIndex < m_nHashSize; ++nIndex)
		{
			CHashItem* pItem = m_pHash[nIndex];
			while (pItem)
			{
				CHashItem* pNext = pItem->m_pNext;
				KEY_INSTANCE_TRAITS::Destructor(pItem->m_Key);
				DATA_INSTANCE_TRAITS::Destructor(pItem->m_Data);
				pItem->m_pNext = m_pGarbage;
				m_pGarbage = pItem;
				pItem = pNext;
			}
			m_pHash[nIndex] = NULL;
		}
	}
	m_nCount = 0;
}

/**
 * @return a @a POSITION value that indicates a starting position for iterating the hash; or NULL if the hash is empty.
 */
template <typename KEY_TYPE, typename DATA_TYPE, class KEY_INSTANCE_TRAITS, class DATA_INSTANCE_TRAITS, class COMPARE_TRAITS, class HASH_TRAITS>
typename CHash<KEY_TYPE, DATA_TYPE, KEY_INSTANCE_TRAITS, DATA_INSTANCE_TRAITS, COMPARE_TRAITS, HASH_TRAITS>::POSITION CHash<KEY_TYPE, DATA_TYPE, KEY_INSTANCE_TRAITS, DATA_INSTANCE_TRAITS, COMPARE_TRAITS, HASH_TRAITS>::GetStartPosition(void) const
{
	POSITION posStart;
	int nIndex = 0;
	while (nIndex < m_nHashSize && m_pHash[nIndex] == NULL)
		++nIndex;
	posStart.m_pItem = nIndex == m_nHashSize ? NULL : m_pHash[nIndex];
	posStart.m_nIndex = nIndex;
	return posStart;
}

/**
 * @param pos - previous position.
 * @return a @a POSITION value that indicates the next position for iterating the hash; or NULL if it's a last position.
 */
template <typename KEY_TYPE, typename DATA_TYPE, class KEY_INSTANCE_TRAITS, class DATA_INSTANCE_TRAITS, class COMPARE_TRAITS, class HASH_TRAITS>
typename CHash<KEY_TYPE, DATA_TYPE, KEY_INSTANCE_TRAITS, DATA_INSTANCE_TRAITS, COMPARE_TRAITS, HASH_TRAITS>::POSITION CHash<KEY_TYPE, DATA_TYPE, KEY_INSTANCE_TRAITS, DATA_INSTANCE_TRAITS, COMPARE_TRAITS, HASH_TRAITS>::GetNextPosition(const typename CHash::POSITION& pos) const
{
	ValidatePosition(pos);
	CHashItem* pItem = pos.m_pItem->m_pNext;
	int nIndex = pos.m_nIndex;
	while (pItem == NULL && ++nIndex < m_nHashSize)
		pItem = m_pHash[nIndex];
	POSITION posNext;
	posNext.m_pItem = pItem;
	posNext.m_nIndex = nIndex;
	return posNext;
}

/**
 * @param pos - hash item position.
 * @return reference to key at specified position.
 */
template <typename KEY_TYPE, typename DATA_TYPE, class KEY_INSTANCE_TRAITS, class DATA_INSTANCE_TRAITS, class COMPARE_TRAITS, class HASH_TRAITS>
inline const KEY_TYPE& CHash<KEY_TYPE, DATA_TYPE, KEY_INSTANCE_TRAITS, DATA_INSTANCE_TRAITS, COMPARE_TRAITS, HASH_TRAITS>::GetKeyAt(const typename CHash::POSITION& pos)
{
	ValidatePosition(pos);
	return pos.m_pItem->m_Key;
}

/**
 * @param pos - hash item position.
 * @return reference to data at specified position.
 */
template <typename KEY_TYPE, typename DATA_TYPE, class KEY_INSTANCE_TRAITS, class DATA_INSTANCE_TRAITS, class COMPARE_TRAITS, class HASH_TRAITS>
inline DATA_TYPE& CHash<KEY_TYPE, DATA_TYPE, KEY_INSTANCE_TRAITS, DATA_INSTANCE_TRAITS, COMPARE_TRAITS, HASH_TRAITS>::GetDataAt(const typename CHash::POSITION& pos)
{
	ValidatePosition(pos);
	return pos.m_pItem->m_Data;
}

/**
 * @param pos - hash item position.
 */
template <typename KEY_TYPE, typename DATA_TYPE, class KEY_INSTANCE_TRAITS, class DATA_INSTANCE_TRAITS, class COMPARE_TRAITS, class HASH_TRAITS>
void CHash<KEY_TYPE, DATA_TYPE, KEY_INSTANCE_TRAITS, DATA_INSTANCE_TRAITS, COMPARE_TRAITS, HASH_TRAITS>::DeleteAt(const typename CHash::POSITION& pos)
{
	ValidatePosition(pos);
	InternalDelete(pos.m_pItem, pos.m_nIndex);
}
