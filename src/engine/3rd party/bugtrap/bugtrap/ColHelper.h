/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Prototypes of collection helpers.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#pragma once

#include "InPlaceNew.h"
#include "StrHolder.h"
#include "StrStream.h"

/// Object instantiation traits.
template <typename DATA_TYPE>
class CObjectTraits
{
public:
	/// Calls the constructor of @a DATA_TYPE class by default.
	static void Constructor(DATA_TYPE& rData);
	/// Calls the destructor of @a DATA_TYPE class by default.
	static void Destructor(DATA_TYPE& rData);
	/// Calls the assignment operator of @a DATA_TYPE class by default.
	static void Assignment(DATA_TYPE& rDstData, const DATA_TYPE& rSrcData);
};

/**
 * @param rData - reference to the memory placement of new object.
 */
template <typename DATA_TYPE>
inline void CObjectTraits<DATA_TYPE>::Constructor(DATA_TYPE& rData)
{
	::new((void*)&rData) DATA_TYPE;
}

/**
 * @param rData - reference to the memory placement of deleted object.
 */
template <typename DATA_TYPE>
inline void CObjectTraits<DATA_TYPE>::Destructor(DATA_TYPE& rData)
{
	rData; rData.~DATA_TYPE();
}

/**
 * @param rDstData - reference to the memory placement of destination object.
 * @param rSrcData - reference to the memory placement of source object.
 */
template <typename DATA_TYPE>
inline void CObjectTraits<DATA_TYPE>::Assignment(DATA_TYPE& rDstData, const DATA_TYPE& rSrcData)
{
	rDstData = rSrcData;
}

/// Value initialization traits.
template <typename DATA_TYPE, DATA_TYPE value = 0>
class CValueTraits
{
public:
	/// Calls the constructor of @a DATA_TYPE by default.
	static void Constructor(DATA_TYPE& rData);
	/// Calls the destructor of @a DATA_TYPE by default
	static void Destructor(DATA_TYPE& rData);
	/// Calls the assignment operator of @a DATA_TYPE by default.
	static void Assignment(DATA_TYPE& rDstData, const DATA_TYPE& rSrcData);
};

/**
 * @param rData - reference to the memory placement of new object.
 */
template <typename DATA_TYPE, DATA_TYPE value>
inline void CValueTraits<DATA_TYPE, value>::Constructor(DATA_TYPE& rData)
{
	rData = value;
}

/**
 * @param rData - reference to the memory placement of deleted object.
 */
template <typename DATA_TYPE, DATA_TYPE value>
inline void CValueTraits<DATA_TYPE, value>::Destructor(DATA_TYPE& /*rData*/)
{
}

/**
 * @param rDstData - reference to the memory placement of destination object.
 * @param rSrcData - reference to the memory placement of source object.
 */
template <typename DATA_TYPE, DATA_TYPE value>
inline void CValueTraits<DATA_TYPE, value>::Assignment(DATA_TYPE& rDstData, const DATA_TYPE& rSrcData)
{
	rDstData = rSrcData;
}

/// Dynamic instantiation traits.
template <typename DATA_TYPE>
class CDynamicTraits
{
public:
	/// Calls the constructor of @a DATA_TYPE class by default.
	static void Constructor(DATA_TYPE& rData);
	/// Calls the destructor of @a DATA_TYPE class by default.
	static void Destructor(DATA_TYPE& rData);
	/// Calls the assignment operator of @a DATA_TYPE class by default.
	static void Assignment(DATA_TYPE& rDstData, const DATA_TYPE& rSrcData);
};

/**
 * @param rData - reference to the memory placement of new object.
 */
template <typename DATA_TYPE>
inline void CDynamicTraits<DATA_TYPE>::Constructor(DATA_TYPE& rData)
{
	rData = NULL;
}

/**
 * @param rData - reference to the memory placement of deleted object.
 */
template <typename DATA_TYPE>
inline void CDynamicTraits<DATA_TYPE>::Destructor(DATA_TYPE& rData)
{
	delete rData;
}

/**
 * @param rDstData - reference to the memory placement of destination object.
 * @param rSrcData - reference to the memory placement of source object.
 */
template <typename DATA_TYPE>
inline void CDynamicTraits<DATA_TYPE>::Assignment(DATA_TYPE& rDstData, const DATA_TYPE& rSrcData)
{
	delete rDstData;
	rDstData = rSrcData;
}

/// Data comparator traits.
template <typename DATA_TYPE>
class CCompareTraits
{
public:
	/// Determines whatever two elements are equal to each other.
	static bool Compare(const DATA_TYPE& rData1, const DATA_TYPE& rData2);
	/// Compares two elements according to their order.
	static int OrderedCompare(const DATA_TYPE& rData1, const DATA_TYPE& rData2);
};

/**
 * @param rData1 - reference to the first element.
 * @param rData2 - reference to the second element.
 * @return true if elements are equal.
 */
template <typename DATA_TYPE>
inline bool CCompareTraits<DATA_TYPE>::Compare(const DATA_TYPE& rData1, const DATA_TYPE& rData2)
{
	return (rData1 == rData2);
}

/**
 * @param rData1 - reference to the first element.
 * @param rData2 - reference to the second element.
 * @return comparison result:
 * - < 0 if first element is less than second element;
 * - = 0 if first element is equal to the second element;
 * - > 0 if first element is greater than second element.
 */
template <typename DATA_TYPE>
int CCompareTraits<DATA_TYPE>::OrderedCompare(const DATA_TYPE& rData1, const DATA_TYPE& rData2)
{
	if (rData1 < rData2)
		return -1;
	else if (rData2 < rData1)
		return +1;
	else
		return 0;
}

/// Explicit template specialization for strings.
template <> class CCompareTraits<TCHAR*>
{
public:
	/// Determines whatever two elements are equal to each other.
	static bool Compare(TCHAR * const & rData1, TCHAR * const & rData2);
	/// Compares two elements according to their order.
	static int OrderedCompare(TCHAR * const & rData1, TCHAR * const & rData2);
};

/**
 * @param rData1 - reference to the first element.
 * @param rData2 - reference to the second element.
 * @return true if elements are equal.
 */
inline bool CCompareTraits<TCHAR*>::Compare(TCHAR * const & rData1, TCHAR * const & rData2)
{
	return (_tcscmp(rData1, rData2) == 0);
}

/**
 * @param rData1 - reference to the first element.
 * @param rData2 - reference to the second element.
 * @return comparison result:
 * - < 0 if first element is less than second element;
 * - = 0 if first element is equal to the second element;
 * - > 0 if first element is greater than second element.
 */
inline int CCompareTraits<TCHAR*>::OrderedCompare(TCHAR * const & rData1, TCHAR * const & rData2)
{
	return _tcscmp(rData1, rData2);
}

/// Explicit template specialization for strings.
template <> class CCompareTraits<const TCHAR*>
{
public:
	/// Determines whatever two elements are equal to each other.
	static bool Compare(TCHAR const * const & rData1, TCHAR const * const & rData2);
	/// Compares two elements according to their order.
	static int OrderedCompare(TCHAR const * const & rData1, TCHAR const * const & rData2);
};

/**
 * @param rData1 - reference to the first element.
 * @param rData2 - reference to the second element.
 * @return true if elements are equal.
 */
inline bool CCompareTraits<const TCHAR*>::Compare(TCHAR const * const & rData1, TCHAR const * const & rData2)
{
	return (_tcscmp(rData1, rData2) == 0);
}

/**
 * @param rData1 - reference to the first element.
 * @param rData2 - reference to the second element.
 * @return comparison result:
 * - < 0 if first element is less than second element;
 * - = 0 if first element is equal to the second element;
 * - > 0 if first element is greater than second element.
 */
inline int CCompareTraits<const TCHAR*>::OrderedCompare(TCHAR const * const & rData1, TCHAR const * const & rData2)
{
	return _tcscmp(rData1, rData2);
}

/// Comparator traits used for incomparable objects.
template <typename DATA_TYPE>
class CIncomparableTraits
{
public:
	/// Determines whatever two elements are equal to each other.
	static bool Compare(const DATA_TYPE& rData1, const DATA_TYPE& rData2);
	/// Compares two elements according to their order.
	static int OrderedCompare(const DATA_TYPE& rData1, const DATA_TYPE& rData2);
};

/**
 * @param rData1 - reference to the first element.
 * @param rData2 - reference to the second element.
 * @return true if elements are equal.
 */
template <typename DATA_TYPE>
inline bool CIncomparableTraits<DATA_TYPE>::Compare(const DATA_TYPE& /*rData1*/, const DATA_TYPE& /*rData2*/)
{
	_ASSERT(FALSE);
	return false;
}

/**
 * @param rData1 - reference to the first element.
 * @param rData2 - reference to the second element.
 * @return comparison result:
 * - < 0 if first element is less than second element;
 * - = 0 if first element is equal to the second element;
 * - > 0 if first element is greater than second element.
 */
template <typename DATA_TYPE>
int CIncomparableTraits<DATA_TYPE>::OrderedCompare(const DATA_TYPE& /*rData1*/, const DATA_TYPE& /*rData2*/)
{
	_ASSERT(FALSE);
	return 0;
}

/// Hash key traits.
template <typename KEY_TYPE>
class CHashTraits
{
public:
	/// Computes the hash value for the given object instance.
	static unsigned HashKey(const KEY_TYPE& key);
};

/**
 * @param key - hash value must be generated for this object.
 * @return hash value of the given object.
 */
template <typename KEY_TYPE>
inline unsigned CHashTraits<KEY_TYPE>::HashKey(const KEY_TYPE& key)
{
	return ((unsigned)key >> 4);
}

/// Explicit template specialization for string hash tables.
template <> class CHashTraits<CStrHolder>
{
public:
	/// Computes the hash value for the given object instance.
	static unsigned HashKey(const CStrHolder& strKey);
};

/**
 * @param strKey - hash value must be generated for this string.
 * @return hash value of the given object.
 */
inline unsigned CHashTraits<CStrHolder>::HashKey(const CStrHolder& strKey)
{
	extern unsigned GetStringHashValue(const TCHAR* pszKey);
	return GetStringHashValue((PCTSTR)strKey);
}

/// Explicit template specialization for string hash tables.
template <> class CHashTraits<CStrStream>
{
public:
	/// Computes the hash value for the given object instance.
	static unsigned HashKey(const CStrStream& strKey);
};

/**
 * @param strKey - hash value must be generated for this string.
 * @return hash value of the given object.
 */
inline unsigned CHashTraits<CStrStream>::HashKey(const CStrStream& strKey)
{
	extern unsigned GetStringHashValue(const TCHAR* pszKey);
	return GetStringHashValue((PCTSTR)strKey);
}

/// Explicit template specialization for string hash tables.
template <> class CHashTraits<TCHAR*>
{
public:
	/// Computes the hash value for the given object instance.
	static unsigned HashKey(TCHAR * const & pszKey);
};

/**
 * @param pszKey - hash value must be generated for this string.
 * @return hash value of the given object.
 */
inline unsigned CHashTraits<TCHAR*>::HashKey(TCHAR * const & pszKey)
{
	extern unsigned GetStringHashValue(const TCHAR* pszKey);
	return GetStringHashValue(pszKey);
}

/// Explicit template specialization for string hash tables.
template <> class CHashTraits<const TCHAR*>
{
public:
	/// Computes the hash value for the given object instance.
	static unsigned HashKey(TCHAR const * const & pszKey);
};

/**
 * @param pszKey - hash value must be generated for this string.
 * @return hash value of the given object.
 */
inline unsigned CHashTraits<const TCHAR*>::HashKey(TCHAR const * const & pszKey)
{
	extern unsigned GetStringHashValue(const TCHAR* pszKey);
	return GetStringHashValue(pszKey);
}
