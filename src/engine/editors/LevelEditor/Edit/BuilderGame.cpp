//----------------------------------------------------
// file: BuilderGame.cpp
//----------------------------------------------------

#include "stdafx.h"
#pragma hdrstop

#include "Builder.h"
#include "Scene.h"
#include "../../xrServerEntities/LevelGameDef.h"
#include "SoundManager_LE.h"
#include "CustomObject.h"
#include "ESceneFogVolumeTools.h"
#include "SpawnPoint.h"

bool sort_fog_vol(EFogVolume* fv1, EFogVolume* fv2)
{
	return (fv1->m_volumeType < fv2->m_volumeType);
}

BOOL SceneBuilder::BuildGame()
{
	SExportStreams 		F;
    F.envmodif.stream.open_chunk	(F.envmodif.chunk++);
    F.envmodif.stream.w_u32			(u32(SPAWNPOINT_VERSION));
    F.envmodif.stream.close_chunk	();
    
    if (!Scene->ExportGame(&F))				return FALSE;

    BOOL bRes 			= TRUE;
    // save spawn
    {
        xr_string lev_spawn 	  			= MakeLevelPath("level.spawn");
        EFS.MarkFile						(lev_spawn.c_str(),true);
        if (F.spawn.chunk)
            if (!F.spawn.stream.save_to		(lev_spawn.c_str())) bRes = FALSE;

        lev_spawn 	  						= MakeLevelPath("level_rs.spawn");
        EFS.MarkFile						(lev_spawn.c_str(),true);
        if (F.spawn_rs.chunk)
            if (!F.spawn_rs.stream.save_to	(lev_spawn.c_str())) bRes = FALSE;
    }

    // save game
    {
        CMemoryWriter GAME; 
        GAME.w_chunk(WAY_PATROLPATH_CHUNK,	F.patrolpath.stream.pointer(),	F.patrolpath.stream.size());
        GAME.w_chunk(RPOINT_CHUNK,			F.rpoint.stream.pointer(),		F.rpoint.stream.size());
        xr_string lev_game 					= MakeLevelPath("level.game");
        EFS.MarkFile						(lev_game.c_str(),true);
        if (GAME.size())
            if (!GAME.save_to				(lev_game.c_str())) bRes = FALSE;
    }

    // save weather env modificator
    {
        xr_string lev_env_mod				= MakeLevelPath("level.env_mod");
        EFS.MarkFile						(lev_env_mod.c_str(),true);
        if (F.envmodif.chunk)
	        if (!F.envmodif.stream.save_to	(lev_env_mod.c_str())) bRes = FALSE;
    }

    // save static sounds
    {
        xr_string lev_sound_static 			= MakeLevelPath("level.snd_static");
        EFS.MarkFile						(lev_sound_static.c_str(),true);
        if (F.sound_static.chunk)    	
            if (!F.sound_static.stream.save_to	(lev_sound_static.c_str())) bRes = FALSE;
    }
/*
    // save sound envs
    {
        xr_string lev_sound_env 			= MakeLevelPath("level.snd_env");
        EFS.MarkFile						(lev_sound_env.c_str(),true);
        if (LSndLib->MakeEnvGeometry		(F.sound_env_geom.stream,false))
            if (!F.sound_env_geom.stream.save_to(lev_sound_env.c_str())) bRes = FALSE;
    }
*/
    // save static PG
    {
        xr_string lev_pe_static 			= MakeLevelPath("level.ps_static");
        EFS.MarkFile						(lev_pe_static.c_str(),true);
        if (F.pe_static.chunk)    	
            if (!F.pe_static.stream.save_to	(lev_pe_static.c_str())) bRes = FALSE;
    }

    // save fog volumes
    if(1)
    {
        xr_string lev_fog_vol 				= MakeLevelPath("level.fog_vol");
        EFS.MarkFile						(lev_fog_vol.c_str(),true);

        F.fog_vol.stream.w_u16				(3); //version

		ObjectList& fogs 					= Scene->ListObj(OBJCLASS_FOG_VOL);

        typedef xr_vector<EFogVolume*> 		tfog_group;
        typedef xr_map<u32, tfog_group> 		tfog_groups;

        tfog_groups							fog_groups;

        for (ObjectIt oit=fogs.begin(); oit!=fogs.end(); ++oit)
        {
            EFogVolume* E 		= dynamic_cast<EFogVolume*>(*oit);
            R_ASSERT			(E);
            u32 grp_id			= E->m_group_id;
            fog_groups[grp_id].push_back(E);
        }

        F.fog_vol.stream.w_u32				(fog_groups.size());

		tfog_groups::iterator git 			= fog_groups.begin();
		tfog_groups::iterator git_e 			= fog_groups.end();
        for(; git!=git_e; ++git)
        {
        	tfog_group& one_group			= git->second;
            std::sort(one_group.begin(), one_group.end(), sort_fog_vol);

            tfog_group::iterator fgit = one_group.begin();
            tfog_group::iterator fgit_e = one_group.end();

            for(; fgit!=fgit_e; ++fgit)
            {
                EFogVolume* E 				= *fgit;
                if(fgit==one_group.begin())
                {
                    if(E->m_volumeType!=fvEmitter)
                    {
                    	bRes 			= FALSE;
                        Msg("! incorrect fog volumes grouping");
						break;
                    }
                    
                    F.fog_vol.stream.w_string	(E->m_volume_profile.c_str());
				}
                Fmatrix M						= E->_Transform();
                F.fog_vol.stream.w				(&M, sizeof(M));

                if(fgit==one_group.begin())
                {
                    if(E->m_volumeType!=fvEmitter)
                    {
                    
                    	bRes 			= FALSE;
                        Msg("! incorrect fog volumes grouping");
						break;
                    }
                    
                    F.fog_vol.stream.w_u32	(one_group.size()-1);
                }else
                {
                
                    if(E->m_volumeType!=fvOcclusion)
                    {
                    	bRes 			= FALSE;
                        Msg("! incorrect fog volumes grouping");
						break;
                    }
                    
                 }   
                    if(!bRes)
                    	break;
            }
            if(!bRes)
                break;
        }

        if (!F.fog_vol.stream.save_to(lev_fog_vol.c_str()))
        	bRes = FALSE;
    }

    return bRes;
}

