#include "stdafx.h"
#pragma hdrstop

#include "EThumbnail.h"
//#include "ImageManager.h"
#pragma package(smart_init)

//------------------------------------------------------------------------------
#define THM_SOUND_VERSION				0x0014
//------------------------------------------------------------------------------
#define THM_CHUNK_SOUNDPARAM			0x1000
#define THM_CHUNK_SOUNDPARAM2			0x1001
#define THM_CHUNK_SOUND_AI_DIST			0x1002
//------------------------------------------------------------------------------
ESoundThumbnail::ESoundThumbnail(LPCSTR src_name, bool bLoad):ECustomThumbnail(src_name,ETSound)
{
    m_fQuality 		= 0.f;
    m_fMinDist 		= 1.f;
    m_fMaxDist		= 300.f;
    m_fMaxAIDist	= 300.f;
    m_fBaseVolume  	= 1.f;
    m_uGameType		= 0;
    if (bLoad) 		Load();
}
//------------------------------------------------------------------------------

ESoundThumbnail::~ESoundThumbnail()
{
}
//------------------------------------------------------------------------------

bool ESoundThumbnail::Load(LPCSTR src_name, LPCSTR path)
{
	string_path 	fn;
    strcpy(fn,EFS.ChangeFileExt(src_name?src_name:m_Name.c_str(),".thm").c_str());
    if (path) 		FS.update_path(fn,path,fn);
    else			FS.update_path(fn,_sounds_,fn);
    if (!FS.exist(fn)) return false;

    IReader* F 		= FS.r_open(fn);
    u16 version 	= 0;

    R_ASSERT(F->r_chunk(THM_CHUNK_VERSION,&version));
    if( version!=THM_SOUND_VERSION ){
		Msg			("!Thumbnail: Unsupported version.");
        return 		false;
    }

    R_ASSERT		(F->find_chunk(THM_CHUNK_TYPE));
    m_Type			= THMType(F->r_u32());
    R_ASSERT		(m_Type==ETSound);

    R_ASSERT		(F->find_chunk(THM_CHUNK_SOUNDPARAM));
    m_fQuality 		= F->r_float();
    m_fMinDist		= F->r_float();
    m_fMaxDist		= F->r_float();
    m_uGameType		= F->r_u32();

    if (F->find_chunk(THM_CHUNK_SOUNDPARAM2))
		m_fBaseVolume	= F->r_float();

    if (F->find_chunk(THM_CHUNK_SOUND_AI_DIST))
    	m_fMaxAIDist= F->r_float();
    else
    	m_fMaxAIDist= m_fMaxDist;
	
    m_Age 			= FS.get_file_age(fn);

    FS.r_close		(F);

    return true;
}
//------------------------------------------------------------------------------

void ESoundThumbnail::Save(int age, LPCSTR path)
{
	if (!Valid()) 	return;

    CMemoryWriter F;
	F.open_chunk	(THM_CHUNK_VERSION);
	F.w_u16			(THM_SOUND_VERSION);
	F.close_chunk	();

    F.open_chunk	(THM_CHUNK_TYPE);
    F.w_u32			(m_Type);
	F.close_chunk	();

    F.open_chunk	(THM_CHUNK_SOUNDPARAM);
    F.w_float		(m_fQuality);
    F.w_float		(m_fMinDist);
    F.w_float		(m_fMaxDist);
    F.w_u32			(m_uGameType);
    F.close_chunk	();

    F.open_chunk	(THM_CHUNK_SOUNDPARAM2);
    F.w_float		(m_fBaseVolume);
    F.close_chunk	();

    F.open_chunk	(THM_CHUNK_SOUND_AI_DIST);
    F.w_float		(m_fMaxAIDist);
    F.close_chunk	();
    
	string_path fn;
    if (path) 		FS.update_path(fn,path,m_Name.c_str());
    else			FS.update_path(fn,_sounds_,m_Name.c_str());

    if (F.save_to(fn))
    {
	    FS.set_file_age	(fn,age?age:m_Age);
    }else{
        Log			("!Can't save thumbnail:",fn);
    }
}
//------------------------------------------------------------------------------

#include "ai_sounds.h"
#include "PropertiesList.h"

bool ESoundThumbnail::OnMaxAIDistAfterEdit(PropValue* sender, float& edit_val)
{
    TProperties* P	= sender->Owner()->Owner(); 		VERIFY(P);
    PropItem* S 	= P->FindItem("Max Dist"); 			VERIFY(S);
    FloatValue* V 	= dynamic_cast<FloatValue*>(S->GetFrontValue());VERIFY(V);
    float max_val 	= V->GetValue	();
	return edit_val<max_val;
}
void ESoundThumbnail::OnMaxDistChange(PropValue* sender)
{
    FloatValue* SV 	= dynamic_cast<FloatValue*>(sender);VERIFY(SV);
    TProperties* P	= sender->Owner()->Owner(); 		VERIFY(P);
    PropItem* S 	= P->FindItem("Max AI Dist"); 		VERIFY(S);
    bool bChanged 	= false;
    for (PropItem::PropValueIt it=S->Values().begin(); S->Values().end() != it; ++it){
	    FloatValue* CV = dynamic_cast<FloatValue*>(*it);VERIFY(CV);
        CV->lim_mx	= *SV->value;
        if (*CV->value>CV->lim_mx){ 
        	ELog.DlgMsg	(mtInformation,"'Max AI Dist' <= 'Max Dist'. 'Max AI Dist' will be clamped.");
        	bChanged	= true;
        	*CV->value 	= CV->lim_mx;
        }
        if (!CV->Equal(S->Values().front()))
            S->m_Flags.set(PropItem::flMixed,TRUE);
    }
	if (bChanged){ 
    	P->Modified		();
        P->RefreshForm	();
    }
}

void ESoundThumbnail::FillProp(PropItemVec& items)
{                                    
	FloatValue* V	= 0;  
    PHelper().CreateFloat		(items, "Quality", 		&m_fQuality);
    PHelper().CreateFloat		(items, "Min Dist",		&m_fMinDist, 	0.01f,1000.f);
    V = PHelper().CreateFloat	(items, "Max Dist",		&m_fMaxDist, 	0.1f,1000.f);
    V->OnChangeEvent.bind		(this,&ESoundThumbnail::OnMaxDistChange);
    V = PHelper().CreateFloat	(items, "Max AI Dist",	&m_fMaxAIDist, 	0.1f,1000.f);
    V->OnAfterEditEvent.bind	(this,&ESoundThumbnail::OnMaxAIDistAfterEdit);
    PHelper().CreateFloat		(items, "Base Volume",	&m_fBaseVolume, 0.f,2.f);
    PHelper().CreateToken32		(items, "Game Type",	&m_uGameType, 	anomaly_type_token);
}
//------------------------------------------------------------------------------

void ESoundThumbnail::FillInfo(PropItemVec& items)
{
    PHelper().CreateCaption		(items, "Quality", 		AnsiString().sprintf("%3.2f",m_fQuality).c_str());
    PHelper().CreateCaption		(items, "Min Dist", 	AnsiString().sprintf("%3.2f",m_fMinDist).c_str());
    PHelper().CreateCaption		(items, "Max Dist",		AnsiString().sprintf("%3.2f",m_fMaxDist).c_str());
    PHelper().CreateCaption		(items, "Max AI Dist",	AnsiString().sprintf("%3.2f",m_fMaxAIDist).c_str());
    PHelper().CreateCaption		(items, "Base Volume",	AnsiString().sprintf("%3.2f",m_fBaseVolume).c_str());
    PHelper().CreateCaption		(items, "Game Type",	get_token_name(anomaly_type_token,m_uGameType));
}
//------------------------------------------------------------------------------

