#include "stdafx.h"
#pragma hdrstop

#include "Scene.h"

#include "ESceneDummyTools.h"

#include "ESceneAIMapTools.h"
#include "ESceneDOTools.h"
#include "ESceneSoundSrcTools.h"
#include "ESceneSoundEnvTools.h"
#include "ESceneGroupTools.h"
#include "ESceneLightTools.h"
#include "ESceneObjectTools.h"
#include "EScenePortalTools.h"
#include "ESceneSectorTools.h"
#include "ESceneGlowTools.h"
#include "EScenePSTools.h"
#include "ESceneShapeTools.h"
#include "ESceneSpawnTools.h"
#include "ESceneWayTools.h"
#include "ESceneWallmarkTools.h"
#include "ESceneFogVolumeTools.h"

void EScene::RegisterSceneTools(ESceneToolBase *mt)
{
    m_SceneTools[mt->ClassID] = mt;
    mt->OnCreate();
}

void EScene::CreateSceneTools()
{
    RegisterSceneTools(new ESceneDummyTool()); //+
    RegisterSceneTools(new ESceneObjectTool()); //+

    RegisterSceneTools(new ESceneLightTool()); //+
    RegisterSceneTools(new ESceneSoundSrcTool()); //+
    RegisterSceneTools(new ESceneSoundEnvTool()); //+
    RegisterSceneTools(new ESceneGroupTool()); //+
    RegisterSceneTools(new ESceneShapeTool()); //+
    RegisterSceneTools(new ESceneGlowTool()); //+
    RegisterSceneTools(new ESceneSpawnTool()); //+
    RegisterSceneTools(new ESceneWayTool()); //+
    RegisterSceneTools(new ESceneSectorTool()); //+
    RegisterSceneTools(new EScenePortalTool()); //+
    RegisterSceneTools(new EScenePSTool()); //+
    RegisterSceneTools(new EDetailManager()); //+
    RegisterSceneTools(new ESceneAIMapTool()); //+
    RegisterSceneTools(new ESceneWallmarkTool()); //+
    RegisterSceneTools(new ESceneFogVolumeTool()); //+
}

void EScene::DestroySceneTools()
{
    SceneToolsMapPairIt _I = m_SceneTools.begin();
    SceneToolsMapPairIt _E = m_SceneTools.end();
    for (; _I!=_E; _I++)
    {
        if (_I->second)
            _I->second->OnDestroy();

        xr_delete(_I->second);
    }
    m_SceneTools.clear();
}

