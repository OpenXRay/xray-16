#include "stdafx.h"

void CLevelPreferences::Load(CInifile *I)
{
    inherited::Load(I);
    {
        OpenObjectList = R_BOOL_SAFE("windows", "object_list", false);
    }
    {
        OpenProperties = R_BOOL_SAFE("windows", "properties", true);
    }
    SceneToolsMapPairIt _I = Scene->FirstTool();
    SceneToolsMapPairIt _E = Scene->LastTool();

    for (; _I != _E; _I++)
        if (_I->second && (_I->first != OBJCLASS_DUMMY))
            _I->second->m_EditFlags.flags = R_U32_SAFE("targets", _I->second->ClassName(), _I->second->m_EditFlags.flags);
}

void CLevelPreferences::Save(CInifile *I)
{
    inherited::Save(I);
    I->w_bool("windows", "object_list", OpenObjectList);
    I->w_bool("windows", "properties", OpenProperties);
    SceneToolsMapPairIt _I = Scene->FirstTool();
    SceneToolsMapPairIt _E = Scene->LastTool();
    for (; _I != _E; _I++)
        if (_I->second && (_I->first != OBJCLASS_DUMMY))
            I->w_u32("targets", _I->second->ClassName(), _I->second->m_EditFlags.get());
}

void CLevelPreferences::OnEnabledChange(PropValue *prop)
{
    ESceneToolBase *M = Scene->GetTool(prop->tag);
    VERIFY(M);
    ExecCommand(COMMAND_ENABLE_TARGET, prop->tag, M->IsEnabled());
}

void CLevelPreferences::OnReadonlyChange(PropValue *prop)
{
    ESceneToolBase *M = Scene->GetTool(prop->tag);
    VERIFY(M);
    ExecCommand(COMMAND_READONLY_TARGET, prop->tag, M->IsForceReadonly());
}

void CLevelPreferences::FillProp(PropItemVec &items)
{
    inherited::FillProp(items);
    SceneToolsMapPairIt _I = Scene->FirstTool();
    SceneToolsMapPairIt _E = Scene->LastTool();
    for (; _I != _E; _I++)
        if (_I->second && (_I->first != OBJCLASS_DUMMY))
        {
            if (_I->second->AllowEnabling())
            {
                PropValue *V = PHelper().CreateFlag32(items, PrepareKey("Scene\\Targets\\Enable", _I->second->ClassDesc()), &_I->second->m_EditFlags, ESceneToolBase::flEnable);
                V->tag = _I->second->FClassID;
                V->OnChangeEvent.bind(this, &CLevelPreferences::OnEnabledChange);
            }
            PropValue *V = PHelper().CreateFlag32(items, PrepareKey("Scene\\Targets\\Read Only", _I->second->ClassDesc()), &_I->second->m_EditFlags, ESceneToolBase::flForceReadonly);
            V->tag = _I->second->FClassID;
            V->OnChangeEvent.bind(this, &CLevelPreferences::OnReadonlyChange);
        }
}
