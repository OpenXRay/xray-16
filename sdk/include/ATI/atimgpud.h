//-----------------------------------------------------------------------------
// File: atimgpud.h
// Copyright (c) 2005 ATI Technologies Inc. All rights reserved.
//-----------------------------------------------------------------------------

#ifndef __ATIMGPUD_H
#define __ATIMGPUD_H

#include <windows.h>

typedef INT (*ATIQUERYMGPUCOUNT)();

#ifdef ATIMGPUD_DLL
	__inline INT AtiMultiGPUAdapters()
	{
		HINSTANCE lib = LoadLibrary(TEXT("ATIMGPUD.DLL"));
		if (!lib)
			return -1;

		ATIQUERYMGPUCOUNT AtiQueryMgpuCount;
		AtiQueryMgpuCount = (ATIQUERYMGPUCOUNT)GetProcAddress(lib, "AtiQueryMgpuCount");
		if (!AtiQueryMgpuCount)
			return -1;

		INT count = AtiQueryMgpuCount();
		if (count < 1) count = 1;

		FreeLibrary(lib);

		return count;
	}
#else
	INT AtiQueryMgpuCount();

	__inline INT AtiMultiGPUAdapters()
	{
		INT count = AtiQueryMgpuCount();
		if (count < 1) count = 1;

		return count;
	}
#endif

#endif // __ATIMGPUD_H
