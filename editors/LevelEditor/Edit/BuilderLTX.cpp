//----------------------------------------------------
// file: BuilderLTX.cpp
//----------------------------------------------------

#include "stdafx.h"
#pragma hdrstop

#include "Builder.h"
#include "Scene.h"
#include "../ECore/Editor/EditObject.h"
#include "SceneObject.h"
#include "ELight.h"
#include "SpawnPoint.h"
#include "WayPoint.h"
#include "xr_ini.h"
#include "xr_efflensflare.h"
#include "GroupObject.h"
#include "EShape.h"
#include "sector.h"

//----------------------------------------------------
BOOL SceneBuilder::ParseLTX(CInifile* pIni, ObjectList& lst, LPCSTR prefix)
{
    return TRUE;
}
//----------------------------------------------------

BOOL SceneBuilder::BuildLTX()
{
	bool bResult	= true;
	int objcount 	= Scene->ObjCount();
	if( objcount <= 0 ) return true;

	xr_string ltx_filename	= MakeLevelPath("level.ltx");

    if (FS.exist(ltx_filename.c_str()))
    	EFS.MarkFile(ltx_filename.c_str(),true);

	// -- defaults --           
    IWriter* F		= FS.w_open(ltx_filename.c_str());

    if (F)
    {
    	string256	buff;
    	F->w_string("[map_usage]");
        sprintf(buff,"ver=%s",Scene->m_LevelOp.m_map_version.c_str());
    	F->w_string(buff);
        if(Scene->m_LevelOp.m_mapUsage.MatchType(eGameIDDeathmatch))
        	F->w_string("deathmatch");
        if(Scene->m_LevelOp.m_mapUsage.MatchType(eGameIDTeamDeathmatch))
        	F->w_string("teamdeathmatch");
        if(Scene->m_LevelOp.m_mapUsage.MatchType(eGameIDArtefactHunt))
        	F->w_string("artefacthunt");
        if(Scene->m_LevelOp.m_mapUsage.MatchType(eGameIDCaptureTheArtefact))
        	F->w_string("capturetheartefact");
        if(Scene->m_LevelOp.m_mapUsage.MatchType(eGameIDDominationZone))
        	F->w_string("dominationzone");
        if(Scene->m_LevelOp.m_mapUsage.MatchType(eGameIDTeamDominationZone))
        	F->w_string("teamdominationzone");

//----
        F->w_string( ";");

        Fbox 	bb;
        Fbox 	bg;
        Scene->GetBox(bb,OBJCLASS_SCENEOBJECT);
        bool r1 = Scene->GetBox(bg,OBJCLASS_GROUP);
        if (r1) bb.merge(bg);
        
		ObjectList& shapes 			= Scene->ListObj(OBJCLASS_SHAPE);
        for (ObjectIt sit=shapes.begin(); sit!=shapes.end(); ++sit)
        {
            CEditShape* E 		= dynamic_cast<CEditShape*>(*sit);
            R_ASSERT			(E);
            if(E->m_shape_type==eShapeLevelBound)
            {
                E->GetBox		(bb);
                break;
            }
        }

        F->w_string		("[level_map]");
        sprintf			(buff,"bound_rect = %f,%f,%f,%f", bb.min.x, bb.min.z,bb.max.x, bb.max.z);
        F->w_string		(buff);
        sprintf			(buff,"texture = map\\map_%s", Scene->m_LevelOp.m_FNLevelPath.c_str());
        F->w_string		(buff);

        
        F->w_string( ";");
        if(Scene->m_LevelOp.m_BOPText.size())
            F->w_stringZ( Scene->m_LevelOp.m_BOPText );

//---- 
        ObjectIt _F = Scene->FirstObj(OBJCLASS_SECTOR);
        ObjectIt _E = Scene->LastObj(OBJCLASS_SECTOR);

        F->w_string		(";");
        F->w_string		("[sub_level_map]");
        for(;_F!=_E;++_F)
        {
            CSector* _S		= (CSector*)(*_F);
            sprintf			(buff,"%d = %d", _S->m_sector_num, _S->m_map_idx);
            F->w_string		(buff);
        }

        FS.w_close	(F);
    }else{
    	bResult 	= false;
    }

	return bResult;
}

