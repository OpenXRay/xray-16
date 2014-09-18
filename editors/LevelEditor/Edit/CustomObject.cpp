//----------------------------------------------------
// file: CustomObject.cpp
//----------------------------------------------------

#include "stdafx.h"
#pragma hdrstop

#include "customobject.h"
#include "UI_LevelMain.h"
#include "../ECore/Editor/D3DUtils.h"
#include "motion.h"
#include "ESceneCustomOTools.h"

#define CUSTOMOBJECT_CHUNK_PARAMS 		0xF900
#define CUSTOMOBJECT_CHUNK_LOCK	 		0xF902
#define CUSTOMOBJECT_CHUNK_TRANSFORM	0xF903
#define CUSTOMOBJECT_CHUNK_GROUP		0xF904
#define CUSTOMOBJECT_CHUNK_MOTION		0xF905
#define CUSTOMOBJECT_CHUNK_FLAGS		0xF906
#define CUSTOMOBJECT_CHUNK_NAME			0xF907
#define CUSTOMOBJECT_CHUNK_MOTION_PARAM	0xF908
//----------------------------------------------------

CCustomObject::CCustomObject(LPVOID data, LPCSTR name)
{
	save_id				= 0;
    ClassID 			= OBJCLASS_DUMMY;
    ParentTool			= 0;
    if (name) 	FName 	= name;
    m_CO_Flags.assign	(0);
    m_RT_Flags.assign	(flRT_Valid|flRT_Visible);
    m_pOwnerObject		= 0;
    ResetTransform		();
    m_RT_Flags.set		(flRT_UpdateTransform,TRUE);
    m_Motion			= NULL;
    m_MotionParams 		= NULL;

    FPosition.set		(0,0,0);
    FScale.set			(1,1,1);
    FRotation.set		(0,0,0);
}


CCustomObject::~CCustomObject()
{
	xr_delete				(m_Motion);
    xr_delete				(m_MotionParams);
}

bool CCustomObject::IsRender()
{
	Fbox bb; GetBox(bb);
    return ::Render->occ_visible(bb)||( Selected() && m_CO_Flags.is_any(flRenderAnyWayIfSelected|flMotion) );
}

void CCustomObject::OnUpdateTransform()
{

	m_RT_Flags.set			(flRT_UpdateTransform,FALSE);
    // update transform matrix

	FTransformR.setXYZi		(-PRotation.x, -PRotation.y, -PRotation.z);
                                
	FTransformS.scale		(PScale);
	FTransformP.translate	(PPosition);
	FTransformRP.mul		(FTransformP,FTransformR);
	FTransform.mul			(FTransformRP,FTransformS);
    FITransformRP.invert	(FTransformRP);
    FITransform.invert		(FTransform);

    if (Motionable()&&Visible()&&Selected()&&m_CO_Flags.is(flAutoKey)) AnimationCreateKey(m_MotionParams->Frame());
}

void CCustomObject::Select( int flag )
{
    if (m_RT_Flags.is(flRT_Visible) && (!!m_RT_Flags.is(flRT_Selected)!=flag))
    {
        m_RT_Flags.set		(flRT_Selected,(flag==-1)?(m_RT_Flags.is(flRT_Selected)?FALSE:TRUE):flag);
        UI->RedrawScene		();
        ExecCommand			(COMMAND_UPDATE_PROPERTIES);
	    ParentTool->OnSelected(this);
    }
}

void CCustomObject::Show( BOOL flag )
{
	m_RT_Flags.set	   	(flRT_Visible,flag);

    if (!m_RT_Flags.is(flRT_Visible)) 
    	m_RT_Flags.set(flRT_Selected, FALSE);
        
    UI->RedrawScene();
};


BOOL   CCustomObject::Editable() const 
{
	BOOL b1 = m_CO_Flags.is(flObjectInGroup);
    BOOL b2 = m_CO_Flags.is(flObjectInGroupUnique);
	return !b1 || (b1&&b2);
}

bool  CCustomObject::LoadLTX(CInifile& ini, LPCSTR sect_name)
{
	m_CO_Flags.assign	(ini.r_u32(sect_name, "co_flags") );

	FName				= ini.r_string(sect_name, "name");
    FPosition			= ini.r_fvector3 	(sect_name, "position");
    VERIFY2				(_valid(FPosition), sect_name);
    FRotation			= ini.r_fvector3 	(sect_name, "rotation");
    VERIFY2				(_valid(FRotation), sect_name);
    FScale				= ini.r_fvector3 	(sect_name, "scale");
    VERIFY2				(_valid(FScale), sect_name);

    // object motion
    if (m_CO_Flags.is(flMotion))
    {
    	m_CO_Flags.set(flMotion, FALSE);
//    	R_ASSERT		(0);
/*
    	VERIFY			(m_Motion);
		F.open_chunk	(CUSTOMOBJECT_CHUNK_MOTION);
		m_Motion->Save	(F);
		F.close_chunk	();
  */
//        m_MotionParams->t_current = ini.r_float		(sect_name, "motion_params_t");
    }
   	LUI->restore_rt_flags	(this);
	return true;
}

bool CCustomObject::LoadStream(IReader& F)
{
    R_ASSERT(F.find_chunk(CUSTOMOBJECT_CHUNK_FLAGS));
    {
        m_CO_Flags.assign(F.r_u32());
    	
        R_ASSERT(F.find_chunk(CUSTOMOBJECT_CHUNK_NAME));
        F.r_stringZ		(FName);
    }

	if(F.find_chunk(CUSTOMOBJECT_CHUNK_TRANSFORM))
    {
        F.r_fvector3(FPosition);
        F.r_fvector3(FRotation);
    VERIFY(_valid(FRotation));
        F.r_fvector3(FScale);
    }

    // object motion
    if (F.find_chunk(CUSTOMOBJECT_CHUNK_MOTION))
    {
        m_Motion 	= xr_new<COMotion>();
        if (!m_Motion->Load(F)){
            ELog.Msg(mtError,"CustomObject: '%s' - motion has different version. Load failed.",Name);
            xr_delete(m_Motion);
        }
        m_MotionParams = xr_new<SAnimParams>();
	    m_MotionParams->Set(m_Motion);
        AnimationUpdate(m_MotionParams->Frame());
    }

    if (F.find_chunk(CUSTOMOBJECT_CHUNK_MOTION_PARAM)){
    	m_MotionParams->t_current = F.r_float();
        AnimationUpdate(m_MotionParams->Frame());
    }

	UpdateTransform	();

   	LUI->restore_rt_flags	(this);
	return true;
}

void CCustomObject::SaveLTX(CInifile& ini, LPCSTR sect_name)
{
	ini.w_u32		(sect_name, "co_flags", m_CO_Flags.get());

	ini.w_string	(sect_name, "name", FName.c_str());

    ini.w_fvector3 	(sect_name, "position", FPosition);
    ini.w_fvector3 	(sect_name, "rotation", FRotation);
    ini.w_fvector3 	(sect_name, "scale", FScale);

/*
    // object motion
    if (m_CO_Flags.is(flMotion))
    {
    	R_ASSERT		(0);

    	VERIFY			(m_Motion);
		F.open_chunk	(CUSTOMOBJECT_CHUNK_MOTION);
		m_Motion->Save	(F);
		F.close_chunk	();

        ini.w_float		(sect_name, "motion_params_t", m_MotionParams->t_current);
    }
*/
   	LUI->store_rt_flags	(this);
}

void CCustomObject::SaveStream(IWriter& F)
{
	F.open_chunk	(CUSTOMOBJECT_CHUNK_FLAGS);
	F.w_u32			(m_CO_Flags.get());
	F.close_chunk	();

	F.open_chunk	(CUSTOMOBJECT_CHUNK_NAME);
	F.w_stringZ		(FName);
	F.close_chunk	();

	F.open_chunk	(CUSTOMOBJECT_CHUNK_TRANSFORM);
    F.w_fvector3 	(FPosition);
    F.w_fvector3 	(FRotation);
    F.w_fvector3 	(FScale);
	F.close_chunk	();

    // object motion
    if (m_CO_Flags.is(flMotion))
    {
    	VERIFY			(m_Motion);
		F.open_chunk	(CUSTOMOBJECT_CHUNK_MOTION);
		m_Motion->Save	(F);
		F.close_chunk	();

        F.open_chunk	(CUSTOMOBJECT_CHUNK_MOTION_PARAM);
        F.w_float		(m_MotionParams->t_current);
        F.close_chunk	();
    }
   	LUI->store_rt_flags	(this);
}
//----------------------------------------------------
#include "ESceneCustomOTools.h"
void CCustomObject::OnFrame()
{
    if (m_Motion) 			AnimationOnFrame();
	if (m_RT_Flags.is(flRT_UpdateTransform)) OnUpdateTransform();
    if(m_CO_Flags.test(flObjectInGroup) && m_pOwnerObject==NULL)
		m_CO_Flags.set(flObjectInGroup, FALSE);
}

void CCustomObject::RenderRoot(int priority, bool strictB2F)
{
	if(FParentTools->IsVisible())
		Render(priority, strictB2F);
}

void CCustomObject::Render(int priority, bool strictB2F)
{
	if ((1==priority)&&(false==strictB2F)){
        if (EPrefs->object_flags.is(epoDrawPivot)&&Selected()){
            DU_impl.DrawObjectAxis(FTransformRP,0.1f,Selected());
        }
        if (m_Motion&&Visible()&&Selected())
            AnimationDrawPath();
    }
}

bool CCustomObject::RaySelect(int flag, const Fvector& start, const Fvector& dir, bool bRayTest){
	float dist = UI->ZFar();
	if ((bRayTest&&RayPick(dist,start,dir))||!bRayTest){
		Select(flag);
        return true;
    }
    return false;
};

bool CCustomObject::FrustumSelect(int flag, const CFrustum& frustum){
	if (FrustumPick(frustum)){
    	Select(flag);
        return true;
    }
	return false;
};

bool CCustomObject::GetSummaryInfo(SSceneSummary* inf)
{
	Fbox bb; 
    if (GetBox(bb)){
        inf->bbox.modify(bb.min);
        inf->bbox.modify(bb.max);
    }
    return true;
}

void CCustomObject::OnSynchronize()
{
	OnFrame		();
}

