#include "pch_script.h"
#include "ai_space.h"
#include "object_factory.h"
#include "ai/monsters/ai_monster_squad_manager.h"
#include "string_table.h"

#include "entity_alive.h"
#include "ui/UIInventoryUtilities.h"
#include "UI/UIXmlInit.h"
#include "UI/UItextureMaster.h"

#include "InfoPortion.h"
#include "PhraseDialog.h"
#include "GameTask.h"
#include "encyclopedia_article.h"

#include "character_info.h"
#include "specific_character.h"
#include "character_community.h"
#include "monster_community.h"
#include "character_rank.h"
#include "character_reputation.h"

#include "profiler.h"

#include "sound_collection_storage.h"
#include "relation_registry.h"

typedef xr_vector<std::pair<shared_str,int> >	STORY_PAIRS;
extern STORY_PAIRS								story_ids;
extern STORY_PAIRS								spawn_story_ids;

extern void show_smart_cast_stats					();
extern void clear_smart_cast_stats					();
extern void release_smart_cast_stats				();
extern void dump_list_wnd							();
extern void dump_list_lines							();
extern void dump_list_sublines						();
extern void clean_wnd_rects							();
extern void dump_list_xmls							();
extern void CreateUIGeom							();
extern void DestroyUIGeom							();
extern void InitHudSoundSettings					();

#include "../xrEngine/IGame_Persistent.h"
void init_game_globals()
{
	CreateUIGeom									();
	InitHudSoundSettings							();
	if(!g_dedicated_server)
	{
//		CInfoPortion::InitInternal					();
//.		CEncyclopediaArticle::InitInternal			();
		CPhraseDialog::InitInternal					();
		InventoryUtilities::CreateShaders			();
	};
	CCharacterInfo::InitInternal					();
	CSpecificCharacter::InitInternal				();
	CHARACTER_COMMUNITY::InitInternal				();
	CHARACTER_RANK::InitInternal					();
	CHARACTER_REPUTATION::InitInternal				();
	MONSTER_COMMUNITY::InitInternal					();
}

extern CUIXml*	g_uiSpotXml;
extern CUIXml*	pWpnScopeXml;

extern void destroy_lua_wpn_params	();

void clean_game_globals()
{
	destroy_lua_wpn_params							();
	// destroy ai space
	xr_delete										(g_ai_space);
	// destroy object factory
	xr_delete										(g_object_factory);
	// destroy monster squad global var
	xr_delete										(g_monster_squad);

	story_ids.clear									();
	spawn_story_ids.clear							();

	if(!g_dedicated_server)
	{
//.		CInfoPortion::DeleteSharedData					();
//.		CInfoPortion::DeleteIdToIndexData				();

//.		CEncyclopediaArticle::DeleteSharedData			();
//.		CEncyclopediaArticle::DeleteIdToIndexData		();

		CPhraseDialog::DeleteSharedData					();
		CPhraseDialog::DeleteIdToIndexData				();
		
		InventoryUtilities::DestroyShaders				();
	}
	CCharacterInfo::DeleteSharedData				();
	CCharacterInfo::DeleteIdToIndexData				();
	
	CSpecificCharacter::DeleteSharedData			();
	CSpecificCharacter::DeleteIdToIndexData			();

	CHARACTER_COMMUNITY::DeleteIdToIndexData		();
	CHARACTER_RANK::DeleteIdToIndexData				();
	CHARACTER_REPUTATION::DeleteIdToIndexData		();
	MONSTER_COMMUNITY::DeleteIdToIndexData			();


	//static shader for blood
	CEntityAlive::UnloadBloodyWallmarks				();
	CEntityAlive::UnloadFireParticles				();
	//очищение памяти таблицы строк
	CStringTable::Destroy							();
	// Очищение таблицы цветов
	CUIXmlInit::DeleteColorDefs						();
	// Очищение таблицы идентификаторов рангов и отношений сталкеров
	InventoryUtilities::ClearCharacterInfoStrings	();

	xr_delete										(g_sound_collection_storage);
	
#ifdef DEBUG
	xr_delete										(g_profiler);
	release_smart_cast_stats						();
#endif

	RELATION_REGISTRY::clear_relation_registry		();

	dump_list_wnd									();
	dump_list_lines									();
	dump_list_sublines								();
	clean_wnd_rects									();
	xr_delete										(g_uiSpotXml);
	dump_list_xmls									();
	DestroyUIGeom									();
	xr_delete										(pWpnScopeXml);
	CUITextureMaster::FreeTexInfo					();
}