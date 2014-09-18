// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//
// Third generation by Oles.

#if !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
#define AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_

#pragma once
#define ENGINE_API 
#define NO_XRC_STATS

#include "../../xrCore/xrCore.h"

#pragma warning(push)
#pragma warning(disable:4995)
#include "d3dx9.h"
#pragma warning(pop)

#pragma comment(lib,"d3dx9.lib")
#pragma comment(lib,"dxerr.lib")

// Warnings
#pragma warning (disable : 4786 )		// too long names
#pragma warning (disable : 4503 )		// decorated name length exceeded
#pragma warning (disable : 4251 )		// object needs DLL interface

#pragma comment(lib,"xrCore.lib")
#pragma comment(lib,"xrQSlim.lib")
#pragma comment(lib,"xrCDB.lib")
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
