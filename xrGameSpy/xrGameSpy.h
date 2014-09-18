// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the XRGAMESPY_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// XRGAMESPY_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef XRGAMESPY_EXPORTS
#define XRGAMESPY_API __declspec(dllexport)
#else
#define XRGAMESPY_API __declspec(dllimport)
#endif
/*
// This class is exported from the xrGameSpy.dll
class XRGAMESPY_API CxrGameSpy {
public:
	CxrGameSpy(void);
	// TODO: add your methods here.
};

extern XRGAMESPY_API int nxrGameSpy;

XRGAMESPY_API int fnxrGameSpy(void);
*/
#pragma once
#include "xrGameSpy_MainDefs.h"

#include "xrGameSpy_Available.h"
#include "xrGameSpy_ServerBrowser.h"
#include "xrGameSpy_QR2.h"
#include "xrGameSpy_CDKey.h"


extern "C"
{
	EXPORT_FN_DECL(const char*, GetGameVersion,		());
	EXPORT_FN_DECL(void,		GetGameID,			(int* GameID, int verID));
}



