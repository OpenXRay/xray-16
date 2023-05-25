//---------------------------------------------------------------------------
#ifndef SoundManager_LEH
#define SoundManager_LEH

#include "../XrECore/Editor/SoundManager.h"

// refs
class ESoundThumbnail;

class CLevelSoundManager : public CSoundManager
{
    typedef CSoundManager inherited;
    bool bNeedRefreshEnvGeom;
    void RealRefreshEnvGeometry();
    void MakeGameSound(ESoundThumbnail *THM, LPCSTR src_name, LPCSTR game_name);

public:
    CLevelSoundManager() { bNeedRefreshEnvGeom = false; }
    ~CLevelSoundManager() { ; }

    virtual void OnFrame();

    void RefreshEnvLibrary();
    void RefreshEnvGeometry() { bNeedRefreshEnvGeom = true; }

    //    bool 		MakeEnvGeometry		(CMemoryWriter& F, bool bErrMsg=false);

    bool Validate();

    void MuteSounds(BOOL bVal);

    void RefreshSounds(bool bSync);

    xr_string UpdateFileName(xr_string &fn);
};

extern CLevelSoundManager *LSndLib;
//---------------------------------------------------------------------------
#endif
