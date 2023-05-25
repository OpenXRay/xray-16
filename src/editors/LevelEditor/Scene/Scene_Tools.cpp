#include "stdafx.h"

void EScene::RegisterSceneTools(ESceneToolBase *mt)
{
    m_SceneTools[mt->FClassID] = mt;
    mt->OnCreate();
}

void EScene::CreateSceneTools()
{
    RegisterSceneTools(xr_new<ESceneDummyTool>());  //+
    RegisterSceneTools(xr_new<ESceneObjectTool>()); //+

    RegisterSceneTools(xr_new<ESceneLightTool>());     //+
    RegisterSceneTools(xr_new<ESceneSoundSrcTool>());  //+
    RegisterSceneTools(xr_new<ESceneSoundEnvTool>());  //+
    RegisterSceneTools(xr_new<ESceneGroupTool>());     //+
    RegisterSceneTools(xr_new<ESceneShapeTool>());     //+
    RegisterSceneTools(xr_new<ESceneGlowTool>());      //+
    RegisterSceneTools(xr_new<ESceneSpawnTool>());     //+
    RegisterSceneTools(xr_new<ESceneWayTool>());       //+
    RegisterSceneTools(xr_new<ESceneSectorTool>());    //+
    RegisterSceneTools(xr_new<EScenePortalTool>());    //+
    RegisterSceneTools(xr_new<EScenePSTool>());        //+
    RegisterSceneTools(xr_new<EDetailManager>());      //+
    RegisterSceneTools(xr_new<ESceneAIMapTool>());     //+
    RegisterSceneTools(xr_new<ESceneWallmarkTool>());  //+
    RegisterSceneTools(xr_new<ESceneFogVolumeTool>()); //+
}

void EScene::DestroySceneTools()
{
    SceneToolsMapPairIt _I = m_SceneTools.begin();
    SceneToolsMapPairIt _E = m_SceneTools.end();
    for (; _I != _E; _I++)
    {
        if (_I->second)
            _I->second->OnDestroy();

        xr_delete(_I->second);
    }
    m_SceneTools.clear();
}
