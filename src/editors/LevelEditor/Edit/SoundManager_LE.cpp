#include "stdafx.h"
#pragma hdrstop

#include "SoundManager_LE.h"
#include "Scene.h"
#include "ESound_Source.h"
#include "ESound_Environment.h"
#include "du_box.h"
#include "xrLevel.h"

CLevelSoundManager*& LSndLib=(CLevelSoundManager*)SndLib;

bool CLevelSoundManager::Validate()
{
	ObjectList& snd_envs = Scene->ListObj(OBJCLASS_SOUND_ENV);
    for (ObjectIt it=snd_envs.begin(); it!=snd_envs.end(); it++){
    	ESoundEnvironment* E = dynamic_cast<ESoundEnvironment*>(*it); R_ASSERT(E);
        if (E->m_EnvInner==E->m_EnvOuter){ 
        	ELog.DlgMsg(mtError,"SoundEnvironment: '%s' inner and outer environment must be different.",E->Name);
        	return false;
        }
    }
	ObjectList& snd_src = Scene->ListObj(OBJCLASS_SOUND_SRC);
    for (it=snd_src.begin(); it!=snd_src.end(); it++){
    	ESoundSource* S = dynamic_cast<ESoundSource*>(*it); R_ASSERT(S);
        if (!S->GetSourceWAV()||(0==strlen(S->GetSourceWAV()))){
        	ELog.DlgMsg(mtError,"SoundSource: '%s' hasn't wave.",S->Name);
        	return false;
        } 
    }
    return true;
}
/*
void CLevelSoundManager::RealRefreshEnvGeometry()
{
	CMemoryWriter F;
	if (MakeEnvGeometry(F,false)){
        IReader R(F.pointer(), F.size());
        ::Sound->set_geometry_env(&R);
    }
}

bool CLevelSoundManager::MakeEnvGeometry(CMemoryWriter& F, bool bErrMsg)
{
	ObjectList& snd_envs = Scene->ListObj(OBJCLASS_SOUND_ENV);

    if (snd_envs.empty()){ 
		if (bErrMsg) ELog.Msg(mtError,"Scene hasn't sound environment geometry.");
    	return false;
    }
    
	RStringVec env_names;

    CDB::Collector CP;
	Fbox aabb; aabb.invalidate();
    for (ObjectIt it=snd_envs.begin(); it!=snd_envs.end(); it++){
    	ESoundEnvironment* E = dynamic_cast<ESoundEnvironment*>(*it); R_ASSERT(E);
        Fbox bb;
        R_ASSERT	(E->GetBox(bb));
        aabb.merge	(bb);

        // get env name indices
        if (E->m_EnvInner==E->m_EnvOuter) continue;
        if ((0==E->m_EnvInner.size())||(0==E->m_EnvOuter.size())) continue;

        int inner = -1;
        int outer = -1;
        for (RStringVecIt e_it=env_names.begin(); e_it!=env_names.end(); e_it++){
        	if ((-1==inner)&&(E->m_EnvInner==*e_it)) inner = e_it-env_names.begin();
        	if ((-1==outer)&&(E->m_EnvOuter==*e_it)) outer = e_it-env_names.begin();
            if ((inner>-1)&&(outer>-1)) break;
        }

        if (-1==inner){ inner=env_names.size(); env_names.push_back(E->m_EnvInner);}
        if (-1==outer){ outer=env_names.size(); env_names.push_back(E->m_EnvOuter);}
        u32 idx = (inner<<16)|(outer);

        // append to collector
		Fmatrix M;	E->get_box	(M);
        
        Fvector bv[DU_BOX_NUMVERTEX];
        for (int k=0; k<DU_BOX_NUMVERTEX; k++) M.transform_tiny(bv[k],du_box_vertices[k]);
    	for (k=0; k<DU_BOX_NUMFACES; k++)
			CP.add_face_packed_D(bv[du_box_faces[k*3+0]],bv[du_box_faces[k*3+1]],bv[du_box_faces[k*3+2]],idx);
    }

    if (env_names.empty()) return false;
    
    // write names
    F.open_chunk	(0);
	for (RStringVecIt e_it=env_names.begin(); e_it!=env_names.end(); e_it++)
    	F.w_stringZ	(e_it->c_str());
    F.close_chunk	();
    
    // write geom
    F.open_chunk	(1);
    // write header
	hdrCFORM		H;
    H.version		= CFORM_CURRENT_VERSION;
    H.vertcount		= CP.getVS();
    H.facecount		= CP.getTS();
    H.aabb			= aabb;
	F.w				(&H,sizeof(hdrCFORM));
    // write vertices&TRIs
    F.w				(CP.getV(),sizeof(Fvector)*H.vertcount);
    F.w				(CP.getT(),sizeof(CDB::TRI)*H.facecount);
    F.close_chunk	();

    return true;
}
*/
void CLevelSoundManager::OnFrame()
{
	inherited::OnFrame();
	if (bNeedRefreshEnvGeom){
    	bNeedRefreshEnvGeom 	= false;
  //      RealRefreshEnvGeometry	();
    }
}
/*
void CLevelSoundManager::RefreshEnvLibrary()
{
	Sound->refresh_env_library();
    RefreshEnvGeometry		();
}
*/
