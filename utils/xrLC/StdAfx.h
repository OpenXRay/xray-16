// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__81632403_DFD8_4A42_A4D3_0AFDD8EA0D25__INCLUDED_)
#define AFX_STDAFX_H__81632403_DFD8_4A42_A4D3_0AFDD8EA0D25__INCLUDED_

#pragma once

//#include "../../xrCore/xrCore.h"

#include "../xrLC_Light/xrLC_Light.h"



#define ENGINE_API				// fake, to enable sharing with engine
//comment - ne figa oni ne sharyatsya

#define ECORE_API				// fake, to enable sharing with editors
#define XR_EPROPS_API
#include "../../xrcore/clsid.h"
#include "defines.h"
#include "cl_log.h"

//#include "_d3d_extensions.h"
//#include "../../editors/LevelEditor/Engine/communicate.h"


#include "b_globals.h"

// TODO: reference additional headers your program requires here

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__81632403_DFD8_4A42_A4D3_0AFDD8EA0D25__INCLUDED_)
