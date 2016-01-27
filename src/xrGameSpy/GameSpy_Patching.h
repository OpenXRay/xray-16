#pragma once

#include "xrCore/xrCore.h"
#include "xrGameSpy/xrGameSpy.h"

class XRGAMESPY_API CGameSpy_Patching
{
public:
    using PatchCheckCallback = fastdelegate::FastDelegate<void(bool success, const char *ver, const char *url)>;
    
	void CheckForPatch	(bool InformOfNoPatch, PatchCheckCallback &cb);
	void PtTrackUsage	(int userID);
};
