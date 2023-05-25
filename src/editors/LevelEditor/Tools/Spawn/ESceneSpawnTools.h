#pragma once

class CEditableObject;

class ESceneSpawnTool : public ESceneCustomOTool
{
    typedef ESceneCustomOTool inherited;
    friend class CSpawnPoint;

protected:
    // controls
    virtual void CreateControls();
    virtual void RemoveControls();
    enum
    {
        flPickSpawnType = (1 << 30),
        flShowSpawnType = (1 << 31),
    };
    Flags32 m_Flags;

    // class
    DEFINE_VECTOR(SChooseItem, SSVec, SSVecIt);
    DEFINE_MAP(CLASS_ID, SSVec, ClassSpawnMap, ClassSpawnMapIt);
    ClassSpawnMap m_Classes;

    // icon list
    DEFINE_MAP(shared_str, ref_shader, ShaderMap, ShaderPairIt);
    ShaderMap m_Icons;
    ref_shader CreateIcon(shared_str name);
    ref_shader GetIcon(shared_str name);
    xr_vector<CEditableObject *> m_draw_RP_visuals;

public:
    ESceneSpawnTool();
    virtual ~ESceneSpawnTool();

    // definition
    IC LPCSTR ClassName() { return "spawn"; }
    IC LPCSTR ClassDesc() { return "Spawn"; }
    IC int RenderPriority() { return 1; }

    void FillProp(LPCSTR pref, PropItemVec &items);

    virtual void Clear(bool bSpecific = false)
    {
        inherited::Clear(bSpecific);
        m_Flags.zero();
    }

    // IO
    virtual bool IsNeedSave() { return true; }
    virtual bool LoadStream(IReader &);
    virtual bool LoadLTX(CInifile &);
    virtual void SaveStream(IWriter &);
    virtual void SaveLTX(CInifile &, int id);
    virtual bool can_use_inifile() { return true; }
    virtual bool LoadSelection(IReader &);
    virtual void SaveSelection(IWriter &);

    virtual int MultiRenameObjects();

    virtual CCustomObject *CreateObject(LPVOID data, LPCSTR name);
    CEditableObject *get_draw_visual(u8 _RP_TeamID, u8 _RP_Type, const GameTypeChooser &_GameType);
};

// refs
class ISE_Abstract;

#include "..\..\XrSE_Factory\xrSE_Factory_import_export.h"
