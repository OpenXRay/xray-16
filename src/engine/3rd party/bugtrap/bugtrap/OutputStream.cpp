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
#include "OutputStream.h"
#include "InputStream.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/**
 * @param arrBytes - array of bytes.
 * @param nCount - size of the array.
 * @return number of written bytes.
 */
int COutputStream::WriteBytes(const unsigned char* arrBytes, int nCount)
{
	int nNumWritten = 0;
	while (nNumWritten < nCount)
	{
		if (! WriteByte(arrBytes[nNumWritten]))
			break;
		++nNumWritten;
	}
	return nNumWritten;
}

/**
 * @param bValue - byte value to be written.
 * @param nCount - number of bytes to write.
 * @return number of written bytes.
 */
int COutputStream::WriteByte(unsigned char bValue, int nCount)
{
	int nNumWritten = 0;
	while (nNumWritten < nCount)
	{
		if (! WriteByte(bValue))
			break;
		++nNumWritten;
	}
	return nNumWritten;
}

/**
 * @param pInputStream - input stream.
 * @return number of written bytes.
 */
int COutputStream::WriteStream(CInputStream* pInputStream)
{
	int nNumWritten = 0;
	for (;;)
	{
		unsigned char arrBuffer[1024];
		int nNumRead = pInputStream->ReadBytes(arrBuffer, sizeof(arrBuffer));
		if (nNumRead <= 0)
			goto end;
		unsigned char* pBuffer = arrBuffer;
		while (nNumRead > 0)
		{
			int nNumWrittenTemp = WriteBytes(pBuffer, nNumRead);
			if (nNumWrittenTemp <= 0)
				goto end;
			nNumRead -= nNumWrittenTemp;
			nNumWritten += nNumWrittenTemp;
			pBuffer += nNumWrittenTemp;
		}
	}
end:
	return nNumWritten;
}
