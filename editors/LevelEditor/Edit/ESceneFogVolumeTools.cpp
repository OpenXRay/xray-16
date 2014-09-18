#include "stdafx.h"
#pragma hdrstop

#include "UI_LevelTools.h"
#include "ESceneFogVolumeTools.h"
#include "FrameFogVol.h"

static const u16 FOG_VOL_TOOLS_VERSION  	= 0x0000;
static const u16 FOG_VOL_VERSION  			= 0x0002;
enum{
    TOOL_CHUNK_VERSION	= 0x1001ul,
	OBJ_CHUNK_VERSION	= 0x1002ul,
	OBJ_CHUNK_DATA		= 0x1003ul
};
xr_token fog_vol_type[] = {
  {"Emitter", fvEmitter},
  {"Occlusion", fvOcclusion},
  {NULL, 0},
};

void ESceneFogVolumeTool::CreateControls()
{
	inherited::CreateDefaultControls(estDefault);
    pFrame 			= xr_new<TfraFogVol>((TComponent*)0,this);
}

void ESceneFogVolumeTool::RemoveControls()
{
	inherited::RemoveControls();
}

void ESceneFogVolumeTool::Clear(bool bSpecific)
{
	inherited::Clear	(bSpecific);
    m_group_counter		= 0;
}

CCustomObject* ESceneFogVolumeTool::CreateObject(LPVOID data, LPCSTR name)
{

	CCustomObject* O	= xr_new<EFogVolume>(data,name);
    O->ParentTool		= this;

    return O;
}

bool ESceneFogVolumeTool::LoadStream(IReader& F)
{
	u16 version 	= 0;
    if(F.r_chunk(TOOL_CHUNK_VERSION,&version))
        if( version!=FOG_VOL_TOOLS_VERSION )
        {
            ELog.DlgMsg( mtError, "%s tools: Unsupported version.",ClassDesc());
            return false;
        }

	if (!inherited::LoadStream(F)) return false;
    return true;
}

void ESceneFogVolumeTool::SaveStream(IWriter& F)
{
	inherited::SaveStream(F);
	F.w_chunk		(TOOL_CHUNK_VERSION,(u16*)&FOG_VOL_TOOLS_VERSION,sizeof(FOG_VOL_TOOLS_VERSION));
}

bool ESceneFogVolumeTool::LoadSelection(IReader& F)
{
	u16 version 	= 0;
    R_ASSERT(F.r_chunk(TOOL_CHUNK_VERSION,&version));
    if( version!=FOG_VOL_TOOLS_VERSION )
    {
        ELog.DlgMsg( mtError, "%s tools: Unsupported version.",ClassDesc());
        return false;
    }

	return inherited::LoadSelection(F);
}

void ESceneFogVolumeTool::SaveSelection(IWriter& F)
{
	F.w_chunk		(TOOL_CHUNK_VERSION,(u16*)&FOG_VOL_TOOLS_VERSION,sizeof(FOG_VOL_TOOLS_VERSION));

	inherited::SaveSelection(F);
}

bool ESceneFogVolumeTool::LoadLTX(CInifile& ini)
{
	u32 version 	= ini.r_u32("main","version");
    if( version!=FOG_VOL_TOOLS_VERSION )
    {
            ELog.DlgMsg( mtError, "%s tools: Unsupported version.",ClassDesc());
            return false;
    }

	inherited::LoadLTX(ini);

	return true;
}

void ESceneFogVolumeTool::SaveLTX(CInifile& ini, int id)
{
	inherited::SaveLTX	(ini, id);
	ini.w_u32		("main", "version", FOG_VOL_TOOLS_VERSION);
}

void ESceneFogVolumeTool::GroupSelected()
{
	++m_group_counter;

    for(ObjectIt it = m_Objects.begin();it!=m_Objects.end();++it)
    {
    	if ((*it)->Selected())
        {
        	EFogVolume* fv 		= (EFogVolume*)(*it);
            fv->m_group_id		= m_group_counter;
        }
    }
}

void ESceneFogVolumeTool::UnGroupCurrent()
{
    for(ObjectIt it = m_Objects.begin();it!=m_Objects.end();++it)
    {
    	if ((*it)->Selected())
        {
        	EFogVolume* fv 		= (EFogVolume*)(*it);
            fv->m_group_id		= u32(-1);
        }
    }

}

void ESceneFogVolumeTool::RegisterGroup(u32 group)
{
   m_group_counter = _max(m_group_counter, group);
}

void ESceneFogVolumeTool::Selected(EFogVolume* fv)
{
	u32 grp 			= fv->m_group_id;
	bool b_sel 			= !!fv->Selected();

    for(ObjectIt it = m_Objects.begin();it!=m_Objects.end();++it)
    {
      	EFogVolume* fv_it 		= (EFogVolume*)(*it);

    	if(b_sel && (fv_it->m_group_id==grp) && (grp!=u32(-1)))
        {
            fv_it->m_DrawEdgeColor = 0xFFFF2020;
        }else
        {
            fv_it->m_DrawEdgeColor = 0xFF202020;
        }
    }

}

//---------------------------------------
EFogVolume::EFogVolume(LPVOID data, LPCSTR name)
	:CEditShape(data,name)
{
	Construct(data);
}

void EFogVolume::Construct(LPVOID data)
{
	ClassID					= OBJCLASS_FOG_VOL;
    m_volumeType			=  fvEmitter;
    m_group_id				= u32(-1);
    m_volume_profile		= "environment\\fog\\default.ltx";
	add_box					(Fidentity);
	SetDrawColor			(0x205050FF, 0xFF202020);
}

EFogVolume::~EFogVolume()
{
}

void EFogVolume::OnUpdateTransform()
{
	inherited::OnUpdateTransform();
}

bool EFogVolume::LoadLTX(CInifile& ini, LPCSTR sect_name)
{
	u32 version 				= ini.r_u32(sect_name, "version");

	inherited::LoadLTX			(ini, sect_name);

    if(version>0)
    {
    	m_volumeType			= ini.r_u8(sect_name, "folume_type");
    	m_group_id				= ini.r_u32(sect_name, "group_id");
    }
    if(version>1 && m_volumeType==fvEmitter)
    	m_volume_profile		= ini.r_string(sect_name,"profile");
        
	OnChangeEnvs				(NULL);

	return 						true;
}

void EFogVolume::SaveLTX(CInifile& ini, LPCSTR sect_name)
{
	inherited::SaveLTX	(ini, sect_name);

	ini.w_u32		(sect_name, "version", FOG_VOL_VERSION);
    ini.w_u8		(sect_name, "folume_type", m_volumeType);
    ini.w_u32		(sect_name, "group_id", m_group_id);

    if(m_volumeType==fvEmitter)
    	ini.w_string	(sect_name, "profile", m_volume_profile.c_str());
}

bool EFogVolume::LoadStream(IReader& F)
{
	u16 version 	= 0;

    R_ASSERT		(F.r_chunk(OBJ_CHUNK_VERSION,&version));

	inherited::LoadStream(F);

    if(F.find_chunk(OBJ_CHUNK_DATA))
    {
        m_volumeType				= F.r_u8();
        m_group_id                  = F.r_u32();
    }
    if(version>1 && m_volumeType==fvEmitter)
		F.r_stringZ					(m_volume_profile);

	OnChangeEnvs					(NULL);
    return true;
}

void EFogVolume::SaveStream(IWriter& F)
{
	inherited::SaveStream	(F);

	F.open_chunk	(OBJ_CHUNK_VERSION);
	F.w_u16			(FOG_VOL_VERSION);
	F.close_chunk	();

	F.open_chunk	(OBJ_CHUNK_DATA);
	F.w_u8			(m_volumeType);
	F.w_u32			(m_group_id);
    if(m_volumeType==fvEmitter)
		F.w_stringZ		(m_volume_profile.c_str());

	F.close_chunk	();
}

void EFogVolume::OnChangeEnvs	(PropValue* prop)
{
	if(m_volumeType == fvEmitter)
		SetDrawColor			(0x205050FF, 0xFF202020);
     else
	if(m_volumeType == fvOcclusion)
		SetDrawColor			(0x2050A050, 0xFF202020);

    ((ESceneFogVolumeTool*)ParentTool)->RegisterGroup(m_group_id);

    LTools->UpdateProperties(FALSE);
}

void EFogVolume::FillProp(LPCSTR pref, PropItemVec& values)
{
	inherited::FillProp			(pref, values);

	PropValue* P;
    P=PHelper().CreateToken8	(values, PrepareKey(pref,"VolumeType"),	&m_volumeType, fog_vol_type);
    P->OnChangeEvent.bind		(this,&EFogVolume::OnChangeEnvs);

    if(m_volumeType==fvEmitter)
    	P=PHelper().CreateRText	(values, PrepareKey(pref,"profile (ltx)"),	&m_volume_profile);
}
//----------------------------------------------------

bool EFogVolume::GetSummaryInfo(SSceneSummary* inf)
{
	inherited::GetSummaryInfo	(inf);
	return true;
}

void EFogVolume::OnSceneUpdate()
{
	inherited::OnSceneUpdate();
}

void EFogVolume::Select(int flag)
{
	inherited::Select(flag);

    if(Selected())
    	((ESceneFogVolumeTool*)ParentTool)->Selected(this);
}

