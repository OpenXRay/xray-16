/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Implementation of collection helpers.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#include "StdAfx.h"
#include "ColHelper.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/**
 * @brief Compute CRC hash value for null terminated string.
 * @param pszKey - hash value must be generated for this object.
 * @return hash value of the given object.
 */
unsigned GetStringHashValue(const TCHAR* pszKey)
{
	TCHAR chCRC = 0;
	while (*pszKey)
	{
		chCRC <<= 1;
		chCRC ^= *pszKey++;
	}
	return chCRC;
}
