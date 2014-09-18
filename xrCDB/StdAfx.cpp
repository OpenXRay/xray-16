// stdafx.cpp : source file that includes just the standard includes
//	xrCDB.pch will be the pre-compiled header
//	stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"
#pragma hdrstop

#ifdef __BORLANDC__
	#pragma comment(lib,"xrCoreB.lib")
#else
	#pragma comment(lib,"xrCore.lib")

	#pragma comment(lib,"xrApi.lib")

#endif

#pragma comment(lib,"winmm.lib")

// TODO: reference any additional headers you need in STDAFX.H
// and not in this file
