#include "pch_script.h"
#include "UIListWnd.h"
#include "UIListItemEx.h"
#include "ServerList.h"
#include "UIMapList.h"
#include "UISpinText.h"
#include "UIMapInfo.h"
#include "UIComboBox.h"

using namespace luabind;

bool CUIListWnd::AddItem_script(CUIListItem* item){
	return AddItem(item, -1);
}

struct CUIListItemWrapper : public CUIListItem, public luabind::wrap_base {};

struct CUIListItemExWrapper : public CUIListItemEx, public luabind::wrap_base {};


#pragma optimize("s",on)
void CUIListWnd::script_register(lua_State *L)
{

	module(L)
	[

		class_<CUIListWnd, CUIWindow>("CUIListWnd")
		.def(							constructor<>())
//		.def("AddText",					&CUIListWnd::AddText_script)
		.def("AddItem",                 &CUIListWnd::AddItem_script, adopt(_2))
		.def("RemoveItem",				&CUIListWnd::RemoveItem)
		.def("RemoveAll",				&CUIListWnd::RemoveAll)
		.def("EnableScrollBar",			&CUIListWnd::EnableScrollBar)
		.def("IsScrollBarEnabled",		&CUIListWnd::IsScrollBarEnabled)
		.def("ScrollToBegin",			&CUIListWnd::ScrollToBegin)
		.def("ScrollToEnd",				&CUIListWnd::ScrollToEnd)
		.def("SetItemHeight",			&CUIListWnd::SetItemHeight)
		.def("GetItem",					&CUIListWnd::GetItem)
		.def("GetItemPos",				&CUIListWnd::GetItemPos)
		.def("GetSize",					&CUIListWnd::GetItemsCount)
		.def("ScrollToBegin",			&CUIListWnd::ScrollToBegin)
		.def("ScrollToEnd",				&CUIListWnd::ScrollToEnd)
		.def("ScrollToPos",				&CUIListWnd::ScrollToPos)
		.def("SetWidth",				&CUIListWnd::SetWidth)
		.def("SetTextColor",			&CUIListWnd::SetTextColor)
		.def("ActivateList",			&CUIListWnd::ActivateList)
		.def("IsListActive",			&CUIListWnd::IsListActive)
		.def("SetVertFlip",				&CUIListWnd::SetVertFlip)
		.def("GetVertFlip",				&CUIListWnd::GetVertFlip)
		.def("SetFocusedItem",			&CUIListWnd::SetFocusedItem)
		.def("GetFocusedItem",			&CUIListWnd::GetFocusedItem)
		.def("ShowSelectedItem",		&CUIListWnd::ShowSelectedItem)

		.def("GetSelectedItem",			&CUIListWnd::GetSelectedItem)
		.def("ResetFocusCapture",		&CUIListWnd::ResetFocusCapture),

		class_<CUIListItem, CUIButton, CUIListItemWrapper>("CUIListItem")
		.def(							constructor<>()),

		class_<CUIListItemEx, CUIListItem/**/, CUIListItemExWrapper/**/>("CUIListItemEx")
		.def(							constructor<>())
		.def("SetSelectionColor",		&CUIListItemEx::SetSelectionColor),

		class_<SServerFilters>("SServerFilters")
		.def(							constructor<>())
		.def_readwrite("empty",				&SServerFilters::empty)
		.def_readwrite("full",				&SServerFilters::full)
		.def_readwrite("with_pass",			&SServerFilters::with_pass)
		.def_readwrite("without_pass",		&SServerFilters::without_pass)
		.def_readwrite("without_ff",		&SServerFilters::without_ff)
#ifdef BATTLEYE
		.def_readwrite("with_battleye",		&SServerFilters::with_battleye)
#endif // BATTLEYE
		.def_readwrite("listen_servers",	&SServerFilters::listen_servers),

		class_<CServerList, CUIWindow>("CServerList")
		.def(							constructor<>())
		.def("ConnectToSelected",		&CServerList::ConnectToSelected)
		.def("SetFilters",				&CServerList::SetFilters)
		.def("SetPlayerName",			&CServerList::SetPlayerName)
		.def("RefreshList",				&CServerList::RefreshGameSpyList)
		.def("RefreshQuick",			&CServerList::RefreshQuick)
		.def("ShowServerInfo",			&CServerList::ShowServerInfo)
		.def("NetRadioChanged",			&CServerList::NetRadioChanged)
		.def("SetSortFunc",				&CServerList::SetSortFunc),
		

		class_<CUIMapList, CUIWindow>("CUIMapList")
		.def(							constructor<>())
		.def("SetWeatherSelector",		&CUIMapList::SetWeatherSelector)
		.def("SetModeSelector",			&CUIMapList::SetModeSelector)
		.def("OnModeChange",			&CUIMapList::OnModeChange)
		.def("LoadMapList",				&CUIMapList::LoadMapList)
		.def("SaveMapList",				&CUIMapList::SaveMapList)
		.def("GetCommandLine",			&CUIMapList::GetCommandLine)
		.def("SetServerParams",			&CUIMapList::SetServerParams)
		.def("GetCurGameType",			&CUIMapList::GetCurGameType)
		.def("StartDedicatedServer",	&CUIMapList::StartDedicatedServer)
		.def("SetMapPic",				&CUIMapList::SetMapPic)
		.def("SetMapInfo",				&CUIMapList::SetMapInfo)
		.def("ClearList",				&CUIMapList::ClearList)
		.def("IsEmpty",					&CUIMapList::IsEmpty),
		
		class_<enum_exporter<EGameIDs> >("GAME_TYPE")
		.enum_("gametype")
		[
			value("GAME_UNKNOWN",			int(-1)),
			value("eGameIDDeathmatch",		int(eGameIDDeathmatch)),
			value("eGameIDTeamDeathmatch",	int(eGameIDTeamDeathmatch)),
			value("eGameIDArtefactHunt",	int(eGameIDArtefactHunt)),
			value("eGameIDCaptureTheArtefact",int(eGameIDCaptureTheArtefact))
		]
		
	];
}