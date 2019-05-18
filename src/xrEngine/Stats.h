// Stats.h: interface for the CStats class.
//
//////////////////////////////////////////////////////////////////////

#pragma once
#include "xrCore/_flags.h"
#include "xrCore/xrstring.h"
#include "xrCommon/xr_vector.h"

class ENGINE_API CGameFont;

DECLARE_MESSAGE(Stats);

class ENGINE_API CStats : public pureRender
{
private:
    CGameFont* statsFont;
    CGameFont* fpsFont;
    float fMem_calls;
    xr_vector<shared_str> errors;

public:
    CStats();
    ~CStats();

    void Show(void);
    virtual void OnRender();
    void OnDeviceCreate(void);
    void OnDeviceDestroy(void);

private:
    void FilteredLog(const char* s);
};

enum
{
    st_sound = (1 << 0),
    st_sound_min_dist = (1 << 1),
    st_sound_max_dist = (1 << 2),
    st_sound_ai_dist = (1 << 3),
    st_sound_info_name = (1 << 4),
    st_sound_info_object = (1 << 5),
};

extern Flags32 g_stats_flags;
