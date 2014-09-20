#include "pch_script.h"
#include "UIGameCustom.h"
#include "level.h"
#include "ui/UIXmlInit.h"
#include "ui/UIStatic.h"
#include "object_broker.h"
#include "string_table.h"

#include "InventoryOwner.h"
#include "ui/UIActorMenu.h"
#include "ui/UIPdaWnd.h"
#include "ui/UIMainIngameWnd.h"
#include "ui/UIMessagesWindow.h"
#include "ui/UIHudStatesWnd.h"
#include "actor.h"
#include "inventory.h"
#include "game_cl_base.h"

#include "../xrEngine/x_ray.h"

EGameIDs ParseStringToGameType(LPCSTR str);

bool predicate_sort_stat(const SDrawStaticStruct* s1, const SDrawStaticStruct* s2) 
{
	return ( s1->IsActual() > s2->IsActual() );
}

struct predicate_find_stat 
{
	LPCSTR	m_id;
	predicate_find_stat(LPCSTR id):m_id(id)	{}
	bool	operator() (SDrawStaticStruct* s) 
	{
		return ( s->m_name==m_id );
	}
};

CUIGameCustom::CUIGameCustom()
:m_msgs_xml(NULL),m_ActorMenu(NULL),m_PdaMenu(NULL),m_window(NULL),UIMainIngameWnd(NULL),m_pMessagesWnd(NULL)
{
	ShowGameIndicators		(true);
	ShowCrosshair			(true);

}
bool g_b_ClearGameCaptions = false;

CUIGameCustom::~CUIGameCustom()
{
	delete_data				(m_custom_statics);
	g_b_ClearGameCaptions	= false;
}


void CUIGameCustom::OnFrame() 
{
	CDialogHolder::OnFrame();
	st_vec_it it = m_custom_statics.begin();
	st_vec_it it_e = m_custom_statics.end();
	for(;it!=it_e;++it)
		(*it)->Update();

	std::sort(	it, it_e, predicate_sort_stat );

	
	while(!m_custom_statics.empty() && !m_custom_statics.back()->IsActual())
	{
		delete_data					(m_custom_statics.back());
		m_custom_statics.pop_back	();
	}
	
	if(g_b_ClearGameCaptions)
	{
		delete_data				(m_custom_statics);
		g_b_ClearGameCaptions	= false;
	}
	m_window->Update();

	//update windows
	if( GameIndicatorsShown() && psHUD_Flags.is(HUD_DRAW|HUD_DRAW_RT) )
		UIMainIngameWnd->Update	();

	m_pMessagesWnd->Update();
}

void CUIGameCustom::Render()
{
	st_vec_it it = m_custom_statics.begin();
	st_vec_it it_e = m_custom_statics.end();
	for(;it!=it_e;++it)
		(*it)->Draw();

	m_window->Draw();

	CEntity* pEntity = smart_cast<CEntity*>(Level().CurrentEntity());
	if (pEntity)
	{
		CActor* pActor			=	smart_cast<CActor*>(pEntity);
		if(pActor && pActor->HUDview() && pActor->g_Alive() && psHUD_Flags.is(HUD_WEAPON|HUD_WEAPON_RT|HUD_WEAPON_RT2))
		{
			u16 ISlot = pActor->inventory().FirstSlot();
			u16 ESlot = pActor->inventory().LastSlot();

			for( ; ISlot<=ESlot; ++ISlot)
			{
				PIItem itm			= pActor->inventory().ItemFromSlot(ISlot);
				if(itm && itm->render_item_ui_query())
					itm->render_item_ui();
			}
		}

		if( GameIndicatorsShown() && psHUD_Flags.is(HUD_DRAW | HUD_DRAW_RT) )
			UIMainIngameWnd->Draw();
	}

	m_pMessagesWnd->Draw();

	DoRenderDialogs();
}

SDrawStaticStruct* CUIGameCustom::AddCustomStatic(LPCSTR id, bool bSingleInstance)
{
	if(bSingleInstance)
	{
		st_vec::iterator it = std::find_if(m_custom_statics.begin(),m_custom_statics.end(), predicate_find_stat(id) );
		if(it!=m_custom_statics.end())
			return (*it);
	}
	
	CUIXmlInit xml_init;
	m_custom_statics.push_back		( xr_new<SDrawStaticStruct>() );
	SDrawStaticStruct* sss			= m_custom_statics.back();

	sss->m_static					= xr_new<CUIStatic>();
	sss->m_name						= id;
	xml_init.InitStatic				(*m_msgs_xml, id, 0, sss->m_static);
	float ttl						= m_msgs_xml->ReadAttribFlt(id, 0, "ttl", -1);
	if(ttl>0.0f)
		sss->m_endTime				= Device.fTimeGlobal + ttl;

	return sss;
}

SDrawStaticStruct* CUIGameCustom::GetCustomStatic(LPCSTR id)
{
	st_vec::iterator it = std::find_if(m_custom_statics.begin(),m_custom_statics.end(), predicate_find_stat(id));
	if(it!=m_custom_statics.end())
		return (*it);

	return NULL;
}

void CUIGameCustom::RemoveCustomStatic(LPCSTR id)
{
	st_vec::iterator it = std::find_if(m_custom_statics.begin(),m_custom_statics.end(), predicate_find_stat(id) );
	if(it!=m_custom_statics.end())
	{
			delete_data				(*it);
		m_custom_statics.erase	(it);
	}
}

void CUIGameCustom::OnInventoryAction(PIItem item, u16 action_type)
{
	if ( m_ActorMenu->IsShown() )
		m_ActorMenu->OnInventoryAction( item, action_type );
}

#include "ui/UIGameTutorial.h"

extern CUISequencer* g_tutorial;
extern CUISequencer* g_tutorial2;

bool CUIGameCustom::ShowActorMenu()
{
	if ( m_ActorMenu->IsShown() )
	{
		m_ActorMenu->HideDialog();
	}else
	{
		HidePdaMenu();
		CInventoryOwner* pIOActor	= smart_cast<CInventoryOwner*>( Level().CurrentViewEntity() );
		VERIFY						(pIOActor);
		m_ActorMenu->SetActor		(pIOActor);
		m_ActorMenu->SetMenuMode	(mmInventory);
		m_ActorMenu->ShowDialog		(true);
	}
	return true;
}

void CUIGameCustom::HideActorMenu()
{
	if ( m_ActorMenu->IsShown() )
	{
		m_ActorMenu->HideDialog();
	}
}

void CUIGameCustom::HideMessagesWindow()
{
	if ( m_pMessagesWnd->IsShown() )
		m_pMessagesWnd->Show(false);
}

void CUIGameCustom::ShowMessagesWindow()
{
	if ( !m_pMessagesWnd->IsShown() )
		m_pMessagesWnd->Show(true);
}

bool CUIGameCustom::ShowPdaMenu()
{
	HideActorMenu();
	m_PdaMenu->ShowDialog(true);
	return true;
}

void CUIGameCustom::HidePdaMenu()
{
	if ( m_PdaMenu->IsShown() )
	{
		m_PdaMenu->HideDialog();
	}
}

void CUIGameCustom::SetClGame(game_cl_GameState* g)
{
	g->SetGameUI(this);
}

void CUIGameCustom::UnLoad()
{
	xr_delete					(m_msgs_xml);
	xr_delete					(m_ActorMenu);
	xr_delete					(m_PdaMenu);
	xr_delete					(m_window);
	xr_delete					(UIMainIngameWnd);
	xr_delete					(m_pMessagesWnd);
}

void CUIGameCustom::Load()
{
	if(g_pGameLevel)
	{
		R_ASSERT				(NULL==m_msgs_xml);
		m_msgs_xml				= xr_new<CUIXml>();
		m_msgs_xml->Load		(CONFIG_PATH, UI_PATH, "ui_custom_msgs.xml");

		R_ASSERT				(NULL==m_ActorMenu);
		m_ActorMenu				= xr_new<CUIActorMenu>		();

		R_ASSERT				(NULL==m_PdaMenu);
		m_PdaMenu				= xr_new<CUIPdaWnd>			();
		
		R_ASSERT				(NULL==m_window);
		m_window				= xr_new<CUIWindow>			();

		R_ASSERT				(NULL==UIMainIngameWnd);
		UIMainIngameWnd			= xr_new<CUIMainIngameWnd>	();
		UIMainIngameWnd->Init	();

		R_ASSERT				(NULL==m_pMessagesWnd);
		m_pMessagesWnd			= xr_new<CUIMessagesWindow>();
		
		Init					(0);
		Init					(1);
		Init					(2);
	}
}

void CUIGameCustom::OnConnected()
{
	if(g_pGameLevel)
	{
		if(!UIMainIngameWnd)
			Load();

		UIMainIngameWnd->OnConnected();
	}
}

void CUIGameCustom::CommonMessageOut(LPCSTR text)
{
	m_pMessagesWnd->AddLogMessage(text);
}
void CUIGameCustom::UpdatePda()
{
	PdaMenu().UpdatePda();
}

void CUIGameCustom::update_fake_indicators(u8 type, float power)
{
	UIMainIngameWnd->get_hud_states()->FakeUpdateIndicatorType(type, power);
}

void CUIGameCustom::enable_fake_indicators(bool enable)
{
	UIMainIngameWnd->get_hud_states()->EnableFakeIndicators(enable);
}

SDrawStaticStruct::SDrawStaticStruct	()
{
	m_static	= NULL;
	m_endTime	= -1.0f;	
}

void SDrawStaticStruct::destroy()
{
	delete_data(m_static);
}

bool SDrawStaticStruct::IsActual() const
{
	if(m_endTime<0)			return true;
	return (Device.fTimeGlobal < m_endTime);
}

void SDrawStaticStruct::SetText(LPCSTR text)
{
	m_static->Show(text!=NULL);
	if(text)
	{
		m_static->TextItemControl()->SetTextST(text);
		m_static->ResetColorAnimation();
	}
}

void SDrawStaticStruct::Draw()
{
	if(m_static->IsShown())
		m_static->Draw();
}

void SDrawStaticStruct::Update()
{
	if(IsActual() && m_static->IsShown())	
		m_static->Update();
}

CMapListHelper	gMapListHelper;
xr_token		game_types[];

void CMapListHelper::LoadMapInfo(LPCSTR map_cfg_fn, const xr_string& map_name, LPCSTR map_ver)
{
	CInifile	ini				(map_cfg_fn);

	shared_str _map_name		= map_name.substr(0,map_name.find('\\')).c_str();
	shared_str _map_ver			= map_ver;

	if(ini.section_exist("map_usage"))
	{
		if(ini.line_exist("map_usage","ver") && !map_ver)
			_map_ver				= ini.r_string("map_usage", "ver");

		CInifile::Sect S			= ini.r_section("map_usage");
		CInifile::SectCIt si		= S.Data.begin();
		CInifile::SectCIt si_e		= S.Data.end();
		for( ;si!=si_e; ++si)
		{
			const shared_str& game_type = (*si).first;
			
			if(game_type=="ver")		continue;

			SGameTypeMaps* M			= GetMapListInt(game_type);
			if(!M)
			{
				Msg						("--unknown game type-%s",game_type.c_str());
				m_storage.resize		(m_storage.size()+1);
				SGameTypeMaps&	Itm		= m_storage.back();
				Itm.m_game_type_name	= game_type;
				Itm.m_game_type_id		= ParseStringToGameType(game_type.c_str());
				M						= &m_storage.back();
			}
			
			SGameTypeMaps::SMapItm	Itm;
			Itm.map_name				= _map_name;
			Itm.map_ver					= _map_ver;
			
			if(M->m_map_names.end()!=std::find(M->m_map_names.begin(),M->m_map_names.end(),Itm))
			{
				Msg("! duplicate map found [%s] [%s]", _map_name.c_str(), _map_ver.c_str());
			}else
			{
#ifndef MASTER_GOLD
				Msg("added map [%s] [%s]", _map_name.c_str(), _map_ver.c_str());
#endif // #ifndef MASTER_GOLD
				M->m_map_names.push_back	(Itm);
			}
		}			
	}

}

void CMapListHelper::Load()
{
	string_path					fn;
	FS.update_path				(fn, "$game_config$", "mp\\map_list.ltx");
	CInifile map_list_cfg		(fn);

	//read weathers set
	CInifile::Sect w			= map_list_cfg.r_section("weather");
	CInifile::SectCIt wi		= w.Data.begin();
	CInifile::SectCIt wi_e		= w.Data.end();
	for( ;wi!=wi_e; ++wi)
	{
		m_weathers.resize		(m_weathers.size()+1);
		SGameWeathers& gw		= m_weathers.back();
		gw.m_weather_name		= (*wi).first;
		gw.m_start_time			= (*wi).second;
	}

	// scan for additional maps
	FS_FileSet			fset;
	FS.file_list		(fset,"$game_levels$",FS_ListFiles,"*level.ltx");

	FS_FileSetIt fit	= fset.begin();
	FS_FileSetIt fit_e	= fset.end();

	for( ;fit!=fit_e; ++fit)
	{
		string_path					map_cfg_fn;
		FS.update_path				(map_cfg_fn, "$game_levels$", (*fit).name.c_str());
		LoadMapInfo					(map_cfg_fn, (*fit).name);
	}
	//scan all not laoded archieves
	LPCSTR tmp_entrypoint			= "temporary_gamedata\\";
	FS_Path* game_levels			= FS.get_path("$game_levels$");
	xr_string prev_root				= game_levels->m_Root;
	game_levels->_set_root			(tmp_entrypoint);

	CLocatorAPI::archives_it it		= FS.m_archives.begin();
	CLocatorAPI::archives_it it_e	= FS.m_archives.end();

	for(;it!=it_e;++it)
	{
		CLocatorAPI::archive& A		= *it;
		if(A.hSrcFile)				continue;

		LPCSTR ln					= A.header->r_string("header", "level_name");
		LPCSTR lv					= A.header->r_string("header", "level_ver");
		FS.LoadArchive				(A, tmp_entrypoint);

		string_path					map_cfg_fn;
		FS.update_path				(map_cfg_fn, "$game_levels$", ln);

		
		xr_strcat					(map_cfg_fn,"\\level.ltx");
		LoadMapInfo					(map_cfg_fn, ln, lv);
		FS.unload_archive			(A);
	}
	game_levels->_set_root			(prev_root.c_str());


	R_ASSERT2	(m_storage.size(), "unable to fill map list");
	R_ASSERT2	(m_weathers.size(), "unable to fill weathers list");
}


const SGameTypeMaps& CMapListHelper::GetMapListFor(const shared_str& game_type)
{
	if( !m_storage.size() )
		Load		();

	return *GetMapListInt(game_type);
}

SGameTypeMaps* CMapListHelper::GetMapListInt(const shared_str& game_type)
{

	TSTORAGE_CIT it		= m_storage.begin();
	TSTORAGE_CIT it_e	= m_storage.end();
	for( ;it!=it_e; ++it)
	{
		if(game_type==(*it).m_game_type_name )
			return &(*it);
	}
	return NULL;
}

const SGameTypeMaps& CMapListHelper::GetMapListFor(const EGameIDs game_id)
{
	if( !m_storage.size() )
	{
		Load		();
		R_ASSERT2	(m_storage.size(), "unable to fill map list");
	}
	TSTORAGE_CIT it		= m_storage.begin();
	TSTORAGE_CIT it_e	= m_storage.end();
	for( ;it!=it_e; ++it)
	{
		if(game_id==(*it).m_game_type_id )
			return (*it);
	}
	return m_storage[0];
}

const GAME_WEATHERS& CMapListHelper::GetGameWeathers() 
{
	if(!m_weathers.size())
		Load();

	return m_weathers;
}

