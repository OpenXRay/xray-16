#include "pch.hpp"
#include "ScriptEngineConfig.hpp"
#include "ScriptExporter.hpp"
#include "xrCore/xrCore.h"


#ifdef XRAY_STATIC_BUILD
#define SCRIPT_EXPORT_PULL(id, dependencies, ...) \
    extern const ScriptExporter::Node id##_ScriptExporterNode; \
    gModulesPull += (intptr_t)&(id##_ScriptExporterNode);

#define SCRIPT_EXPORT_FUNC_PULL(id, dependencies, func) SCRIPT_EXPORT_PULL(id, dependencies, func)


namespace XRay::ScriptExportDetails {
static size_t gModulesPull = 0;

// List of nodes was gathered by the command:
// grep -r --include="*.cpp" -h --context=0 --no-group-separator SCRIPT_EXPORT > script_exports.txt
static void pull_export_nodes()
{
    SCRIPT_EXPORT_PULL(CScriptSoundType, ());
    SCRIPT_EXPORT_PULL(CSE_ALifeItemPDA, (CSE_ALifeItem));
    SCRIPT_EXPORT_PULL(CSE_ALifeItemDocument, (CSE_ALifeItem));
    SCRIPT_EXPORT_PULL(CSE_ALifeItemGrenade, (CSE_ALifeItem));
    SCRIPT_EXPORT_PULL(CSE_ALifeItemExplosive, (CSE_ALifeItem));
    SCRIPT_EXPORT_PULL(CSE_ALifeItemBolt, (CSE_ALifeItem));
    SCRIPT_EXPORT_PULL(CSE_ALifeItemCustomOutfit, (CSE_ALifeItem));
    SCRIPT_EXPORT_PULL(CSE_ALifeItemHelmet, (CSE_ALifeItem));
    SCRIPT_EXPORT_PULL(CSE_ALifeItemWeaponMagazined, (CSE_ALifeItemWeapon));
    SCRIPT_EXPORT_PULL(CScriptRTokenList, ());
    SCRIPT_EXPORT_PULL(CPureServerObject, ());
    SCRIPT_EXPORT_PULL(CSE_Abstract, (CPureServerObject));
    SCRIPT_EXPORT_PULL(CSE_Shape, ());
    SCRIPT_EXPORT_PULL(CSE_Visual, ());
    SCRIPT_EXPORT_PULL(CSE_Motion, ());
    SCRIPT_EXPORT_PULL(CSE_Spectator, (CSE_Abstract));
    SCRIPT_EXPORT_PULL(CSE_Temporary, (CSE_Abstract));
    SCRIPT_EXPORT_PULL(Fcolor, ());
    SCRIPT_EXPORT_PULL(CSE_ALifeObjectHangingLamp, (CSE_ALifeDynamicObjectVisual, CSE_PHSkeleton));
    SCRIPT_EXPORT_PULL(CSE_ALifeObjectPhysic, (CSE_ALifeDynamicObjectVisual, CSE_PHSkeleton));
    SCRIPT_EXPORT_PULL(CSE_ALifeSmartZone, (CSE_ALifeSpaceRestrictor, CSE_ALifeSchedulable));
    SCRIPT_EXPORT_PULL(ClientID, ());
    SCRIPT_EXPORT_PULL(NET_Packet, ());
    SCRIPT_EXPORT_PULL(CSE_PHSkeleton, ());
    SCRIPT_EXPORT_PULL(CSE_AbstractVisual, (CSE_Visual, CSE_Abstract));
    //SCRIPT_EXPORT_PULL(CSE_SpawnGroup, (CSE_Abstract));
    SCRIPT_EXPORT_PULL(Flags8, ());
    SCRIPT_EXPORT_PULL(Flags16, ());
    SCRIPT_EXPORT_PULL(Flags32, ());
    SCRIPT_EXPORT_PULL(CObjectFactory, ());
    SCRIPT_EXPORT_PULL(CSE_ALifeInventoryItem, ());
    SCRIPT_EXPORT_PULL(CSE_ALifeItem, (CSE_ALifeDynamicObjectVisual, CSE_ALifeInventoryItem));
    SCRIPT_EXPORT_PULL(CSE_ALifeItemTorch, (CSE_ALifeItem));
    SCRIPT_EXPORT_PULL(CSE_ALifeItemAmmo, (CSE_ALifeItem));
    SCRIPT_EXPORT_PULL(CSE_ALifeItemWeapon, (CSE_ALifeItem));
    SCRIPT_EXPORT_PULL(CSE_ALifeItemWeaponShotGun, (CSE_ALifeItemWeapon));
    SCRIPT_EXPORT_PULL(CSE_ALifeItemWeaponAutoShotGun, (CSE_ALifeItemWeapon));
    SCRIPT_EXPORT_PULL(CSE_ALifeItemDetector, (CSE_ALifeItem));
    SCRIPT_EXPORT_PULL(CSE_ALifeItemArtefact, (CSE_ALifeItem));
    SCRIPT_EXPORT_FUNC_PULL(CSE_ALifeMonsterAbstract, (CSE_ALifeCreatureAbstract, CSE_ALifeSchedulable), CSE_ALifeMonsterAbstract_Export);
    SCRIPT_EXPORT_FUNC_PULL(CSE_ALifeHumanAbstract, (CSE_ALifeTraderAbstract, CSE_ALifeMonsterAbstract), CSE_ALifeHumanAbstract_Export);
    SCRIPT_EXPORT_PULL(CSE_ALifePsyDogPhantom, (CSE_ALifeMonsterBase));
    SCRIPT_EXPORT_PULL(IReader, ());
    SCRIPT_EXPORT_PULL(CSE_ALifeCreatureCrow, (CSE_ALifeCreatureAbstract));
    SCRIPT_EXPORT_PULL(CSE_ALifeMonsterZombie, (CSE_ALifeMonsterAbstract));
    SCRIPT_EXPORT_PULL(CSE_ALifeMonsterBase, (CSE_ALifeMonsterAbstract, CSE_PHSkeleton));
    SCRIPT_EXPORT_PULL(CSE_ALifeHumanStalker, (CSE_ALifeHumanAbstract, CSE_PHSkeleton));
    SCRIPT_EXPORT_FUNC_PULL(CScriptIniFile, (), CScriptIniFile_Export);
    SCRIPT_EXPORT_PULL(Fvector, ());
    SCRIPT_EXPORT_PULL(Fvector2, ());
    SCRIPT_EXPORT_PULL(Fbox, ());
    SCRIPT_EXPORT_PULL(Frect, ());
    SCRIPT_EXPORT_FUNC_PULL(CSE_SmartCover, (CSE_ALifeDynamicObject), CSE_SmartCover_Export);
    SCRIPT_EXPORT_PULL(CSE_ALifeItemWeaponMagazinedWGL, (CSE_ALifeItemWeaponMagazined));
    SCRIPT_EXPORT_PULL(CSE_ALifeObjectProjector, (CSE_ALifeDynamicObjectVisual));
    SCRIPT_EXPORT_PULL(CSE_ALifeHelicopter, (CSE_ALifeDynamicObjectVisual, CSE_Motion, CSE_PHSkeleton));
    SCRIPT_EXPORT_PULL(CSE_ALifeCar, (CSE_ALifeDynamicObjectVisual, CSE_PHSkeleton));
    SCRIPT_EXPORT_PULL(CSE_ALifeObjectBreakable, (CSE_ALifeDynamicObjectVisual));
    SCRIPT_EXPORT_PULL(CSE_ALifeObjectClimable, (CSE_Shape, CSE_Abstract));
    SCRIPT_EXPORT_PULL(CSE_ALifeMountedWeapon, (CSE_ALifeDynamicObjectVisual));
    SCRIPT_EXPORT_PULL(CSE_ALifeTeamBaseZone, (CSE_ALifeSpaceRestrictor));
    SCRIPT_EXPORT_PULL(CSE_ALifeSchedulable, ());
    SCRIPT_EXPORT_PULL(CSE_ALifeGraphPoint, (CSE_Abstract));
    SCRIPT_EXPORT_PULL(CSE_ALifeObject, (CSE_Abstract));
    SCRIPT_EXPORT_PULL(CSE_ALifeGroupAbstract, ());
    SCRIPT_EXPORT_PULL(CSE_ALifeDynamicObject, (CSE_ALifeObject));
    SCRIPT_EXPORT_PULL(CSE_ALifeDynamicObjectVisual, (CSE_ALifeDynamicObject, CSE_Visual));
    SCRIPT_EXPORT_PULL(CSE_ALifePHSkeletonObject, (CSE_ALifeDynamicObjectVisual, CSE_PHSkeleton));
    SCRIPT_EXPORT_PULL(CSE_ALifeSpaceRestrictor, (CSE_ALifeDynamicObject, CSE_Shape));
    SCRIPT_EXPORT_PULL(CSE_ALifeLevelChanger, (CSE_ALifeSpaceRestrictor));
    SCRIPT_EXPORT_PULL(CSE_ALifeInventoryBox, (CSE_ALifeDynamicObjectVisual));
    SCRIPT_EXPORT_PULL(Fmatrix, ());
    SCRIPT_EXPORT_PULL(CSE_ALifeTraderAbstract, ());
    SCRIPT_EXPORT_PULL(CSE_ALifeTraderAbstract, ());
    SCRIPT_EXPORT_PULL(CSE_ALifeTrader, (CSE_ALifeDynamicObjectVisual, CSE_ALifeTraderAbstract));
    SCRIPT_EXPORT_PULL(CSE_ALifeCustomZone, (CSE_ALifeDynamicObject, CSE_Shape));
    SCRIPT_EXPORT_PULL(CSE_ALifeAnomalousZone, (CSE_ALifeCustomZone));
    SCRIPT_EXPORT_PULL(CSE_ALifeMonsterRat, (CSE_ALifeMonsterAbstract, CSE_ALifeInventoryItem));
    SCRIPT_EXPORT_PULL(CScriptTokenList, ());
    SCRIPT_EXPORT_PULL(CSE_ALifeCreatureActor, (CSE_ALifeCreatureAbstract, CSE_ALifeTraderAbstract, CSE_PHSkeleton));
    SCRIPT_EXPORT_PULL(CSE_ALifeTorridZone, (CSE_ALifeCustomZone, CSE_Motion));
    SCRIPT_EXPORT_PULL(CSE_ALifeZoneVisual, (CSE_ALifeAnomalousZone, CSE_Visual));
    SCRIPT_EXPORT_PULL(CSE_ALifeCreaturePhantom, (CSE_ALifeCreatureAbstract));
    SCRIPT_EXPORT_PULL(CSE_ALifeCreatureAbstract, (CSE_ALifeDynamicObjectVisual));
    SCRIPT_EXPORT_FUNC_PULL(CSE_ALifeOnlineOfflineGroup, (CSE_ALifeDynamicObject, CSE_ALifeSchedulable), CSE_ALifeOnlineOfflineGroup_Export);
    SCRIPT_EXPORT_PULL(Device, ());
    SCRIPT_EXPORT_PULL(KeyBindings, ());
    SCRIPT_EXPORT_PULL(CheckRendererSupport, ());
    //FIXME SCRIPT_EXPORT_PULL(CScriptPropertiesListHelper, ());
    SCRIPT_EXPORT_PULL(CUIComboBox, (CUIWindow));
    SCRIPT_EXPORT_PULL(CUIListWnd, (CUIWindow));
    SCRIPT_EXPORT_PULL(CUIListItem, (CUIButton));
    SCRIPT_EXPORT_PULL(CUIListItemEx, (CUIListItem));
    SCRIPT_EXPORT_PULL(CUITabControl, (CUIWindow));
    SCRIPT_EXPORT_PULL(CUITabButton, (CUIButton));
    SCRIPT_EXPORT_PULL(UICore, ());
    SCRIPT_EXPORT_PULL(UIStyleManager, ());
    SCRIPT_EXPORT_PULL(CUITextureMaster, ());
    SCRIPT_EXPORT_PULL(CUIWindow, ());
    SCRIPT_EXPORT_PULL(CUIFrameWindow, (CUIWindow));
    SCRIPT_EXPORT_PULL(CUIFrameLineWnd, (CUIWindow));
    SCRIPT_EXPORT_PULL(UIHint, (CUIWindow));
    SCRIPT_EXPORT_PULL(CUIScrollView, (CUIWindow));
    SCRIPT_EXPORT_PULL(EnumUIMessages, ());
    SCRIPT_EXPORT_PULL(CUIButton, (CUIStatic, CUIWindow));
    SCRIPT_EXPORT_PULL(CUIOptionsManagerScript, ());
    SCRIPT_EXPORT_PULL(CUIProgressBar, (CUIWindow));
    SCRIPT_EXPORT_PULL(CUIPropertiesBox, (CUIFrameWindow));
    SCRIPT_EXPORT_PULL(CUIMessageBox, (CUIStatic));
    SCRIPT_EXPORT_PULL(CUILines, ());
    SCRIPT_EXPORT_PULL(CUIStatic, (CUIWindow));
    SCRIPT_EXPORT_PULL(CUITextWnd, (CUIWindow));
    SCRIPT_EXPORT_PULL(CUIListBox, (CUIScrollView));
    SCRIPT_EXPORT_PULL(CUIListBoxItem, (CUIFrameLineWnd));
    SCRIPT_EXPORT_PULL(CUIListBoxItemMsgChain, (CUIListBoxItem));
    SCRIPT_EXPORT_PULL(CUIEditBox, (CUIWindow));
    SCRIPT_EXPORT_PULL(CPhysicObject, (CGameObject));
    SCRIPT_EXPORT_PULL(CHangingLamp, (CGameObject));
    SCRIPT_EXPORT_PULL(CHolderCustom, ());
    SCRIPT_EXPORT_PULL(CPHCallOnStepCondition, ());
    SCRIPT_EXPORT_PULL(CPHExpireOnStepCondition, (CPHCallOnStepCondition));
    SCRIPT_EXPORT_PULL(CPHConstForceAction, ());
    SCRIPT_EXPORT_PULL(CScriptZone, (IFactoryObject));
    SCRIPT_EXPORT_PULL(CSmartZone, (IFactoryObject));
    SCRIPT_EXPORT_PULL(CEF_Storage, ());
    SCRIPT_EXPORT_PULL(CGameTask, ());
    SCRIPT_EXPORT_PULL(CScriptParticleAction, ());
    SCRIPT_EXPORT_PULL(game_sv_Deathmatch, (game_sv_GameState));
    SCRIPT_EXPORT_PULL(login_manager, ());
    SCRIPT_EXPORT_PULL(profile, ());
    SCRIPT_EXPORT_PULL(login_operation_cb, ());
    SCRIPT_EXPORT_PULL(CCar, (CGameObject, CHolderCustom));
    SCRIPT_EXPORT_PULL(CScriptSound, ());
    SCRIPT_EXPORT_PULL(SZoneMapEntityData, ());
    SCRIPT_EXPORT_PULL(SZoneMapEntities, ());
    SCRIPT_EXPORT_PULL(RPoint, ());
    SCRIPT_EXPORT_PULL(game_cl_GameState, (game_GameState));
    SCRIPT_EXPORT_PULL(game_cl_mp, (game_cl_GameState));
    SCRIPT_EXPORT_FUNC_PULL(game_cl_mp_script, (game_cl_mp), game_cl_mp_script_script_register);
    SCRIPT_EXPORT_PULL(CScope, (CGameObject));
    SCRIPT_EXPORT_FUNC_PULL(CScriptActionPlannerAction, (CScriptActionPlanner, CScriptActionBase), CScriptActionPlannerAction_Export);
    SCRIPT_EXPORT_PULL(CALifeMonsterDetailPathManager, ());
    SCRIPT_EXPORT_PULL(profile_store, ());
    SCRIPT_EXPORT_PULL(CPhraseDialogExporter, ());
    SCRIPT_EXPORT_PULL(game_sv_GameState, (game_GameState));
    SCRIPT_EXPORT_PULL(EGameEnums, ());
    SCRIPT_EXPORT_PULL(CPseudoGigant, (CGameObject));
    SCRIPT_EXPORT_PULL(CPoltergeist, (CGameObject));
    SCRIPT_EXPORT_PULL(CController, (CGameObject));
    SCRIPT_EXPORT_PULL(CAI_Dog, (CGameObject));
    SCRIPT_EXPORT_PULL(CZombie, (CGameObject));
    SCRIPT_EXPORT_PULL(CSnork, (CGameObject));
    SCRIPT_EXPORT_PULL(CAI_Boar, (CGameObject));
    SCRIPT_EXPORT_PULL(CChimera, (CGameObject));
    SCRIPT_EXPORT_PULL(CAI_Bloodsucker, (CGameObject));
    SCRIPT_EXPORT_PULL(CFracture, (CGameObject));
    SCRIPT_EXPORT_PULL(CCat, (CGameObject));
    SCRIPT_EXPORT_PULL(CBurer, (CGameObject));
    SCRIPT_EXPORT_PULL(CAI_Flesh, (CGameObject));
    SCRIPT_EXPORT_PULL(CTushkano, (CGameObject));
    SCRIPT_EXPORT_PULL(CAI_PseudoDog, (CGameObject));
    SCRIPT_EXPORT_PULL(CPsyDog, (CGameObject));
    SCRIPT_EXPORT_PULL(CPsyDogPhantom, (CGameObject));
    SCRIPT_EXPORT_PULL(CAI_Trader, (CGameObject));
    SCRIPT_EXPORT_PULL(CStalkerPlanner, (CScriptActionPlanner));
    SCRIPT_EXPORT_PULL(CAI_Stalker, (CGameObject));
    SCRIPT_EXPORT_PULL(CScriptSoundInfo, ());
    SCRIPT_EXPORT_PULL(game_PlayerState, ());
    SCRIPT_EXPORT_FUNC_PULL(game_GameState, (IFactoryObject), game_GameState_script_register);
    SCRIPT_EXPORT_PULL(CALifeHumanBrain, (CALifeMonsterBrain));
    SCRIPT_EXPORT_FUNC_PULL(CScriptActionBase, (), CScriptActionBase_Export);
    SCRIPT_EXPORT_PULL(CHelicopter, (CGameObject));
    SCRIPT_EXPORT_PULL(CScriptActionCondition, ());
    SCRIPT_EXPORT_PULL(CMincer, (CGameObject));
    SCRIPT_EXPORT_PULL(CScriptEntityAction, ());
    SCRIPT_EXPORT_PULL(CScriptHit, ());
    SCRIPT_EXPORT_FUNC_PULL(CLevel, (), CLevel_Export);
    SCRIPT_EXPORT_PULL(demo_player_info, ());
    SCRIPT_EXPORT_PULL(demo_info, ());
    SCRIPT_EXPORT_PULL(CWeapon, (CGameObject));
    SCRIPT_EXPORT_PULL(CWeaponMagazined, (CWeapon));
    SCRIPT_EXPORT_PULL(CWeaponMagazinedWGrenade, (CWeaponMagazined));
    SCRIPT_EXPORT_PULL(CWeaponBinoculars, (CWeaponMagazined));
    SCRIPT_EXPORT_PULL(CWeaponBM16, (CWeaponShotgun));
    SCRIPT_EXPORT_PULL(CWeaponFN2000, (CWeaponMagazined));
    SCRIPT_EXPORT_PULL(CWeaponFORT, (CWeaponMagazined));
    SCRIPT_EXPORT_PULL(CF1, (CGameObject, CExplosive));
    SCRIPT_EXPORT_PULL(CWeaponHPSA, (CWeaponMagazined));
    SCRIPT_EXPORT_PULL(CWeaponKnife, (CWeapon));
    SCRIPT_EXPORT_PULL(CWeaponLR300, (CWeaponMagazined));
    SCRIPT_EXPORT_PULL(CWeaponPM, (CWeaponMagazined));
    SCRIPT_EXPORT_PULL(CRGD5, (CGameObject, CExplosive));
    SCRIPT_EXPORT_PULL(CWeaponRPG7, (CWeaponMagazined));
    SCRIPT_EXPORT_PULL(CWeaponSVD, (CWeaponMagazined));
    SCRIPT_EXPORT_PULL(CWeaponSVU, (CWeaponMagazined));
    SCRIPT_EXPORT_PULL(CWeaponAK74, (CWeaponMagazinedWGrenade));
    SCRIPT_EXPORT_PULL(CWeaponAutomaticShotgun, (CWeaponMagazined));
    SCRIPT_EXPORT_PULL(CWeaponGroza, (CWeaponMagazinedWGrenade));
    SCRIPT_EXPORT_PULL(CWeaponRG6, (CWeaponShotgun));
    SCRIPT_EXPORT_PULL(CWeaponShotgun, (CWeaponMagazined));
    SCRIPT_EXPORT_PULL(CWeaponUSP45, (CWeaponMagazined));
    SCRIPT_EXPORT_PULL(CWeaponVal, (CWeaponMagazined));
    SCRIPT_EXPORT_PULL(CWeaponVintorez, (CWeaponMagazined));
    SCRIPT_EXPORT_PULL(CWeaponWalther, (CWeaponMagazined));
    SCRIPT_EXPORT_PULL(IFactoryObject, ());
    SCRIPT_EXPORT_PULL(ISheduled, ());
    SCRIPT_EXPORT_PULL(IRenderable, ());
    SCRIPT_EXPORT_PULL(ICollidable, ());
    SCRIPT_EXPORT_PULL(CGameObject, (IFactoryObject, ISheduled, ICollidable, IRenderable));
    SCRIPT_EXPORT_PULL(IRenderVisual, ());
    SCRIPT_EXPORT_PULL(IKinematicsAnimated, ());
    SCRIPT_EXPORT_PULL(CBlend, ());
    SCRIPT_EXPORT_PULL(CScriptPropertyEvaluator, ());
    SCRIPT_EXPORT_PULL(CPropertyStorage, ());
    SCRIPT_EXPORT_PULL(CSpaceRestrictor, (CGameObject));
    SCRIPT_EXPORT_PULL(profile_data_script_registrator, ());
    SCRIPT_EXPORT_PULL(store_operation_cb, ());
    SCRIPT_EXPORT_PULL(CMapManager, ());
    SCRIPT_EXPORT_PULL(CMapLocation, ());
    SCRIPT_EXPORT_FUNC_PULL(UIRegistrator, (CDialogHolder), UIRegistratorScriptExport)
    SCRIPT_EXPORT_FUNC_PULL(CMemoryInfo, (), CMemoryInfo_Export);
    SCRIPT_EXPORT_PULL(CScriptWatchAction, ());
    SCRIPT_EXPORT_PULL(CScriptParticles, ());
    SCRIPT_EXPORT_FUNC_PULL(CCustomOutfit, (CGameObject), CCustomOutfit_Export);
    SCRIPT_EXPORT_FUNC_PULL(CHelmet, (CGameObject), CHelmet_Export);
    SCRIPT_EXPORT_PULL(account_manager, ());
    SCRIPT_EXPORT_PULL(suggest_nicks_cb, ());
    SCRIPT_EXPORT_PULL(account_operation_cb, ());
    SCRIPT_EXPORT_PULL(account_profiles_cb, ());
    SCRIPT_EXPORT_PULL(found_email_cb, ());
    SCRIPT_EXPORT_PULL(CScriptGameObject, ());
    SCRIPT_EXPORT_PULL(CScriptMonsterHitInfo, ());
    SCRIPT_EXPORT_FUNC_PULL(CActor, (CGameObject), CActor_Export);
    SCRIPT_EXPORT_PULL(cphysics_world_scripted, ());
    SCRIPT_EXPORT_PULL(CScriptEffector, ());
    SCRIPT_EXPORT_PULL(CConsole, ());
    SCRIPT_EXPORT_PULL(CSavedGameWrapper, ());
    SCRIPT_EXPORT_PULL(CEntityCondition, ());
    SCRIPT_EXPORT_PULL(CActorCondition, (CEntityCondition));
    SCRIPT_EXPORT_PULL(lanim_wrapper, ());
    SCRIPT_EXPORT_PULL(CScriptAnimationAction, ());
    SCRIPT_EXPORT_PULL(cphysics_element_scripted, ());
    SCRIPT_EXPORT_PULL(smart_cover_object, (CGameObject));
    SCRIPT_EXPORT_PULL(CScriptMonsterAction, ());
    SCRIPT_EXPORT_PULL(CALifeMonsterMovementManager, ());
    SCRIPT_EXPORT_PULL(fs_registrator, ());
    SCRIPT_EXPORT_PULL(cphysics_shell_scripted, ());
    //SCRIPT_EXPORT_PULL(CPhysicsElement, ());
    //SCRIPT_EXPORT_PULL(CPhysicsJoint, ());
    SCRIPT_EXPORT_PULL(CDialogHolder, (CUIWindow));
    SCRIPT_EXPORT_PULL(CUIDialogWnd, (CDialogHolder));
    SCRIPT_EXPORT_PULL(CUIMessageBoxEx, (CUIDialogWnd));
    SCRIPT_EXPORT_PULL(CUIMMShniaga, (CUIWindow));
    SCRIPT_EXPORT_PULL(CUISleepStatic, (CUIStatic));
    SCRIPT_EXPORT_PULL(SServerFilters, ());
    SCRIPT_EXPORT_PULL(connect_error_cb, ());
    SCRIPT_EXPORT_PULL(CServerList, (CUIWindow));
    SCRIPT_EXPORT_PULL(CUIMapList, (CUIWindow));
    SCRIPT_EXPORT_PULL(CUIVersionList, (CUIWindow));
    SCRIPT_EXPORT_PULL(EnumGameIDs, ());
    SCRIPT_EXPORT_PULL(CUIActorMenu, (CUIDialogWnd));
    SCRIPT_EXPORT_PULL(CUIPdaWnd, (CUIDialogWnd));
    SCRIPT_EXPORT_PULL(FactionState, ());
    SCRIPT_EXPORT_PULL(CUIListBox, (CUIScrollView));
    SCRIPT_EXPORT_PULL(CUIListBoxItem, (CUIFrameLineWnd));
    SCRIPT_EXPORT_PULL(CUIListBoxItemMsgChain, (CUIListBoxItem));
    SCRIPT_EXPORT_PULL(SServerFilters, ());
    SCRIPT_EXPORT_PULL(connect_error_cb, ());
    SCRIPT_EXPORT_PULL(CServerList, (CUIWindow));
    SCRIPT_EXPORT_PULL(CUIMapList, (CUIWindow));
    SCRIPT_EXPORT_PULL(EnumGameIDs, ());
    SCRIPT_EXPORT_PULL(CUIDialogWndEx, (CUIDialogWnd, IFactoryObject));
    SCRIPT_EXPORT_PULL(CUIMapInfo, (CUIWindow));
    SCRIPT_EXPORT_PULL(FractionState, ());
    SCRIPT_EXPORT_PULL(CStalkerOutfit, (CGameObject));
    SCRIPT_EXPORT_PULL(CTorch, (CGameObject));
    SCRIPT_EXPORT_PULL(CScriptSoundAction, ());
    SCRIPT_EXPORT_PULL(cphysics_joint_scripted, ());
    SCRIPT_EXPORT_PULL(CMosquitoBald, (CGameObject));
    SCRIPT_EXPORT_PULL(CScriptGameDifficulty, ());
    SCRIPT_EXPORT_PULL(CCoverPoint, ());
    SCRIPT_EXPORT_PULL(CALifeSmartTerrainTask, ());
    SCRIPT_EXPORT_PULL(CALifeMonsterBrain, ());
    SCRIPT_EXPORT_PULL(CScriptObjectAction, ());
    SCRIPT_EXPORT_FUNC_PULL(game_sv_mp, (game_sv_GameState), game_sv_mp_script_register);
    SCRIPT_EXPORT_FUNC_PULL(game_sv_mp_script, (game_sv_mp), game_sv_mp_script_script_register);
    SCRIPT_EXPORT_PULL(CParticleParams, ());
    SCRIPT_EXPORT_PULL(CALifeSimulator, ());
    SCRIPT_EXPORT_PULL(CUIGameCustom, (CDialogHolder));
    SCRIPT_EXPORT_PULL(CRenderDevice, ());
    SCRIPT_EXPORT_PULL(CArtefact, (CGameObject));
    SCRIPT_EXPORT_PULL(CHairsZone, (CGameObject));
    SCRIPT_EXPORT_PULL(CAI_Crow, (CGameObject));
    SCRIPT_EXPORT_PULL(CALifeMonsterPatrolPathManager, ());
    SCRIPT_EXPORT_PULL(CScriptMovementAction, ());
    SCRIPT_EXPORT_PULL(CClientSpawnManager, ());
    SCRIPT_EXPORT_PULL(CExplosive, ());
    SCRIPT_EXPORT_PULL(CScriptXmlInit, ());
    SCRIPT_EXPORT_FUNC_PULL(CScriptActionPlanner, (), CScriptActionPlanner_Export);
    SCRIPT_EXPORT_PULL(CScriptBinderObject, ());
    SCRIPT_EXPORT_PULL(CGameGraph, ());
    SCRIPT_EXPORT_PULL(CPatrolPathParams, ());
    SCRIPT_EXPORT_PULL(CScriptWorldProperty, ());
    SCRIPT_EXPORT_PULL(CScriptWorldState, ());
    SCRIPT_EXPORT_PULL(CScriptEngine, ());
}
}
#endif

using namespace XRay;

ScriptExporter::Node* ScriptExporter::Node::firstNode = nullptr;
ScriptExporter::Node* ScriptExporter::Node::lastNode = nullptr;
size_t ScriptExporter::Node::nodeCount = 0;

ScriptExporter::Node::Node(const char* id, size_t depCount, const char* const* deps, ExporterFunc exporterFunc)
{
    this->id = id;
    this->depCount = depCount;
    this->deps = deps;
    this->exporterFunc = exporterFunc;
    done = false;
    InsertAfter(nullptr, this);
}

ScriptExporter::Node::~Node()
{
    // Remap locals
    // ... <-> N <-> this <-> N <-> ...
    {
        if (prevNode)
            prevNode->nextNode = this->nextNode;

        if (nextNode)
            nextNode->prevNode = this->prevNode;
    }

    // Remap globals
    {
        // this <-> N <-> ...
        if (firstNode == this)
            firstNode = this->nextNode;

        // ... <-> N <-> this
        if (lastNode == this)
            lastNode = this->prevNode;
    }
}

void ScriptExporter::Node::Export(lua_State* luaState)
{
    if (done)
    {
#ifdef CONFIG_SCRIPT_ENGINE_LOG_SKIPPED_EXPORTS
        Msg("* ScriptExporter: skipping exported node %s", id);
#endif
        return;
    }
    // export dependencies recursively
    for (size_t i = 0; i < depCount; i++)
    {
        // check if 'deps[i]' depends on 'node'
        for (Node* n = GetFirst(); n; n = n->GetNext())
        {
            if (!n->done && !strcmp(deps[i], n->id))
            {
                n->Export(luaState);
                break;
            }
        }
    }
#ifdef CONFIG_SCRIPT_ENGINE_LOG_EXPORTS
    Msg("* ScriptExporter: exporting node %s", id);
#endif
    exporterFunc(luaState);
    done = true;
}

bool ScriptExporter::Node::HasDependency(const Node* node) const
{
    for (size_t i = 0; i < depCount; i++)
    {
        if (!strcmp(deps[i], node->id))
            return true;
    }
    for (size_t i = 0; i < depCount; i++)
    {
        // check if 'deps[i]' depends on 'node'
        for (Node* n = GetFirst(); n; n = n->GetNext())
        {
            if (!strcmp(deps[i], n->id))
            {
                if (n->HasDependency(node))
                    return true;
                break;
            }
        }
    }
    return false;
}

void ScriptExporter::Node::InsertAfter(Node* target, Node* node)
{
    if (!target)
    {
        node->prevNode = nullptr;
        node->nextNode = firstNode;
        if (firstNode)
            firstNode->prevNode = node;
        else
            lastNode = node;
        firstNode = node;
    }
    else
    {
        node->prevNode = target;
        node->nextNode = target->nextNode;
        if (target == lastNode)
            lastNode = node;
        target->nextNode = node;
    }
    nodeCount++;
}

void ScriptExporter::Export(lua_State* luaState)
{
#ifdef XRAY_STATIC_BUILD
    ScriptExportDetails::pull_export_nodes();
#endif

#ifdef CONFIG_SCRIPT_ENGINE_LOG_EXPORTS
    Msg("* ScriptExporter: total nodes: %zu", Node::GetCount());
    for (auto node = Node::GetFirst(); node; node = node->GetNext())
    {
        Msg("* %s", node->GetId());
        size_t depCount = node->GetDependencyCount();
        const char* const* depIds = node->GetDependencyIds();
        for (int i = 0; i < depCount; i++)
            Msg("* <- %s", depIds[i]);
    }
#endif
    for (auto node = Node::GetFirst(); node; node = node->GetNext())
        node->Export(luaState);
}

void ScriptExporter::Reset()
{
    for (auto node = Node::GetFirst(); node; node = node->GetNext())
        node->Reset();
}
