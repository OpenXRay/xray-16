#include "stdafx.h"
#pragma hdrstop

#include "ESceneCustomMTools.h"
#include "editors/ECore/Editor/ui_main.h"
#include "UI_LevelMain.h"
#include "scene.h"

#define CHUNK_TOOLS_TAG 0x7777

ESceneToolBase::ESceneToolBase(ObjClassID cls)
{
    ClassID = cls;
    // controls
    sub_target = 0;
    pCurControl = 0;
    pFrame = 0;
    action = -1;
    m_EditFlags.assign(flEnable | flVisible);
    m_ModifName = "";
    m_ModifTime = 0;
}

ESceneToolBase::~ESceneToolBase() {}
void ESceneToolBase::Clear(bool bSpecific)
{
    m_ModifName = "";
    m_ModifTime = 0;
}

void ESceneToolBase::Reset()
{
    Clear();
    m_EditFlags.set(flReadonly, FALSE);
}

void ESceneToolBase::OnCreate()
{
    OnDeviceCreate();
    CreateControls();
}

void ESceneToolBase::OnDestroy()
{
    OnDeviceDestroy();
    RemoveControls();
}

bool ESceneToolBase::LoadLTX(CInifile& ini)
{
    m_ModifName = ini.r_string("modif", "name");
    m_ModifTime = ini.r_u32("modif", "time");
    return true;
}

void ESceneToolBase::SaveLTX(CInifile& ini, int id)
{
    ini.w_string("modif", "name", m_ModifName.c_str());
    ini.w_u32("modif", "time", m_ModifTime);
}

bool ESceneToolBase::LoadStream(IReader& F)
{
    if (F.find_chunk(CHUNK_TOOLS_TAG))
    {
        F.r_stringZ(m_ModifName);
        F.r(&m_ModifTime, sizeof(m_ModifTime));
    }
    else
    {
        m_ModifName = "";
        m_ModifTime = 0;
    }
    return true;
}

void ESceneToolBase::SaveStream(IWriter& F)
{
    /*
        xr_string mn	= AnsiString().sprintf("\\\\%s\\%s",Core.CompName,Core.UserName).c_str();
        time_t mt		= time(NULL);

        F.open_chunk(CHUNK_TOOLS_TAG);
        F.w_stringZ	(mn);
        F.w			(&mt,sizeof(mt));
        F.close_chunk();
    */
}
