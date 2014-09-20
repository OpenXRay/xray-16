//---------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "ESound_Environment.h"
#include "EShape.h"
#include "ui_levelmain.h"
//----------------------------------------------------

#define SOUND_SEL0_COLOR 	0x00A0A0A0
#define SOUND_SEL1_COLOR 	0x00FFFFFF
#define SOUND_NORM_COLOR 	0x000000FF
#define SOUND_LOCK_COLOR 	0x00FF0000
//----------------------------------------------------

#define SOUND_ENV_VERSION  				0x0012
//----------------------------------------------------
#define SOUND_CHUNK_VERSION				0x1001
#define SOUND_CHUNK_ENV_SHAPE			0x1002
#define SOUND_CHUNK_ENV_REFS			0x1003
//----------------------------------------------------

ESoundEnvironment::ESoundEnvironment(LPVOID data, LPCSTR name)
	:CEditShape(data,name)
{
	Construct(data);
}

void ESoundEnvironment::Construct(LPVOID data)
{
	ClassID					= OBJCLASS_SOUND_ENV;
    
	add_box					(Fidentity);
	SetDrawColor			(0x205050FF, 0xFF202020);
    m_EnvInner				= "";
    m_EnvOuter				= "";
}

ESoundEnvironment::~ESoundEnvironment()
{
}
//----------------------------------------------------

void ESoundEnvironment::OnUpdateTransform()
{
	inherited::OnUpdateTransform();
    ExecCommand		(COMMAND_REFRESH_SOUND_ENV_GEOMETRY);
}
//----------------------------------------------------
bool ESoundEnvironment::LoadLTX(CInifile& ini, LPCSTR sect_name)
{
	u32 version 	= ini.r_u32(sect_name, "version");

    if(version!=SOUND_ENV_VERSION)
    {
        ELog.DlgMsg	(mtError, "ESoundSource: Unsupported version.");
        return 		false;
    }
	inherited::LoadLTX			(ini, sect_name);

	return 			true;
}

void ESoundEnvironment::SaveLTX(CInifile& ini, LPCSTR sect_name)
{
	inherited::SaveLTX	(ini, sect_name);

	ini.w_u32		(sect_name, "version", SOUND_ENV_VERSION);

    ini.w_string	(sect_name, "env_inner", m_EnvInner.c_str());
    ini.w_string	(sect_name, "env_outer", m_EnvOuter.c_str());
}

bool ESoundEnvironment::LoadStream(IReader& F)
{
	u16 version 	= 0;

    R_ASSERT(F.r_chunk(SOUND_CHUNK_VERSION,&version));
    if(version!=SOUND_ENV_VERSION){
        ELog.DlgMsg( mtError, "ESoundSource: Unsupported version.");
        return false;
    }
	inherited::LoadStream			(F);

    R_ASSERT(F.find_chunk(SOUND_CHUNK_ENV_REFS));
    F.r_stringZ				(m_EnvInner);
    F.r_stringZ				(m_EnvOuter);

    return true;
}

void ESoundEnvironment::SaveStream(IWriter& F)
{
	inherited::SaveStream	(F);

	F.open_chunk	(SOUND_CHUNK_VERSION);
	F.w_u16			(SOUND_ENV_VERSION);
	F.close_chunk	();

    F.open_chunk	(SOUND_CHUNK_ENV_REFS);
    F.w_stringZ		(m_EnvInner);
    F.w_stringZ		(m_EnvOuter);
    F.close_chunk	();
}
//----------------------------------------------------

void ESoundEnvironment::OnChangeEnvs	(PropValue* prop)
{
	ExecCommand		(COMMAND_REFRESH_SOUND_ENV_GEOMETRY);
}
//----------------------------------------------------

void ESoundEnvironment::FillProp(LPCSTR pref, PropItemVec& values)
{
	inherited::FillProp			(pref, values);
	PropValue* P;
    P=PHelper().CreateChoose	(values, PrepareKey(pref,"Environment Inner"),	&m_EnvInner, smSoundEnv);
    P->OnChangeEvent.bind		(this,&ESoundEnvironment::OnChangeEnvs);
    P=PHelper().CreateChoose	(values, PrepareKey(pref,"Environment Outer"),	&m_EnvOuter, smSoundEnv);
    P->OnChangeEvent.bind		(this,&ESoundEnvironment::OnChangeEnvs);
}
//----------------------------------------------------

bool ESoundEnvironment::GetSummaryInfo(SSceneSummary* inf)
{
	inherited::GetSummaryInfo	(inf);
	return true;
}

void ESoundEnvironment::get_box(Fmatrix& m)
{
	CShapeData::shape_def shape = get_shape(0);
    R_ASSERT(shape.type==CShapeData::cfBox);
    m.mul				(_Transform(),shape.data.box);
}

void ESoundEnvironment::OnSceneUpdate()
{
	inherited::OnSceneUpdate();
	ExecCommand( COMMAND_REFRESH_SOUND_ENV_GEOMETRY );
}

