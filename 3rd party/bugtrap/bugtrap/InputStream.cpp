/*
 * This is a part of the BugTrap package.
 * Copyright (c) 2005-2007 IntelleSoft.
 * All rights reserved.
 *
 * Description: Input stream.
 * Author: Maksim Pyatkovskiy.
 *
 * This source code is only intended as a supplement to the
 * BugTrap package reference and related electronic documentation
 * provided with the product. See these sources for detailed
 * information regarding the BugTrap package.
 */

#include "StdAfx.h"
#include "InputStream.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/**
 * @param arrBytes - array of bytes.
 * @param nCount - size of the array.
 * @return number of retrieved bytes.
 */
int CInputStream::ReadBytes(unsigned char* arrBytes, int nCount)
{
	int nNumRead = 0;
	while (nNumRead < nCount)
	{
		if (! ReadByte(arrBytes[nNumRead]))
			break;
		++nNumRead;
	}
	return nNumRead;
}
