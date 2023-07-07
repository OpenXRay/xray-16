// Engine.h: interface for the CEngine class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ENGINE_H__22802DD7_D7EB_4234_9781_E237657471AC__INCLUDED_)
#define AFX_ENGINE_H__22802DD7_D7EB_4234_9781_E237657471AC__INCLUDED_
#pragma once

#include "ELog.h"
//#include "../../xrCPU_Pipe/xrCPU_Pipe.h"

// TODO: this should be in render configuration
#define R__NUM_SUN_CASCADES         (3u) // csm/s.ligts
#define R__NUM_AUX_CONTEXTS         (1u) // rain/s.lights
#define R__NUM_PARALLEL_CONTEXTS    (R__NUM_SUN_CASCADES + R__NUM_AUX_CONTEXTS)
#define R__NUM_CONTEXTS             (R__NUM_PARALLEL_CONTEXTS + 1/* imm */)

class ENGINE_API CEngine
{
	HMODULE hPSGP;

public:
	CEngine();
	~CEngine();

	void Initialize();
	void Destroy();
	LPCSTR LastWindowsError();

	void ReloadSettings();
};

//ENGINE_API extern xrDispatchTable PSGP;
ENGINE_API extern CEngine Engine;

#endif // !defined(AFX_ENGINE_H__22802DD7_D7EB_4234_9781_E237657471AC__INCLUDED_)
