//---------------------------------------------------------------------------
#ifndef SHSoundEnvToolsH
#define SHSoundEnvToolsH
#include "SHToolsInterface.h"
#include "../../xrSound/soundrender_environment.h"

// refs
class PropValue;

class CSHSoundEnvTools : public ISHTools
{
    void ItemExist(LPCSTR name, bool &res) { res = !!FindItem(name); }
    CSoundRender_Environment *FindItem(LPCSTR name);
    void SetCurrentEnv(CSoundRender_Environment *B);

    SoundEnvironment_LIB m_Library;

    shared_str m_SoundName;
    CSound_params m_Params;
    ref_sound m_PreviewSnd;
    void OnControlClick(ButtonValue *sender, bool &bModif, bool &bSafe);
    void OnChangeWAV(PropValue *prop);

    void OnRevResetClick(ButtonValue *sender, bool &bModif, bool &bSafe);
    void OnEnvSizeChange(PropValue *sender);
    void OnEnvChange(PropValue *sender);

public:
    CSoundRender_Environment m_EnvSrc;
    CSoundRender_Environment *m_Env;
    virtual void AppendItem(LPCSTR folder_name, LPCSTR parent = 0);
    virtual void OnRemoveItem(LPCSTR name, EItemType type);
    virtual void OnRenameItem(LPCSTR old_full_name, LPCSTR new_full_name, EItemType type);
    virtual void FillItemList();

    void UseEnvironment() { Sound->set_user_env(m_Env); }

public:
    CSHSoundEnvTools(const ISHInit &init);
    virtual ~CSHSoundEnvTools();

    virtual LPCSTR ToolsName() { return "Sound Environment"; }

    virtual void Reload();
    virtual void Load();
    virtual bool Save();

    virtual bool OnCreate();
    virtual void OnDestroy();
    virtual void OnActivate();
    virtual void OnDeactivate();

    // misc
    virtual void ResetCurrentItem();
    virtual void SetCurrentItem(LPCSTR name, bool bView);
    virtual void ApplyChanges(bool bForced = false);

    virtual void RealUpdateProperties();
    virtual void RealUpdateList();

    virtual void OnFrame();
    virtual void OnRender();

    virtual void OnDeviceCreate() { ; }
    virtual void OnDeviceDestroy() { ; }
};
//---------------------------------------------------------------------------

#endif
