#include "pch_script.h"
#include "UIDialogWnd.h"
#include "UIDialogHolder.h"
#include "GamePersistent.h"
#include "UIMessageBoxEx.h"
#include "UILabel.h"
#include "UIMMShniaga.h"
#include "UISleepStatic.h"
#include "ServerList.h"
#include "UIMapInfo.h"
#include "xrUICore/ComboBox/UIComboBox.h"
#include "UIMapList.h"
#include "ScriptXMLInit.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;
using namespace luabind::policy;

// clang-format off
SCRIPT_EXPORT(CDialogHolder, (CUIWindow), {
    module(luaState)
    [
        class_<CDialogHolder>("CDialogHolder")
            .def("TopInputReceiver", &CDialogHolder::TopInputReceiver)
            .def("MainInputReceiver", &CDialogHolder::TopInputReceiver)
            .def("start_stop_menu", &CDialogHolder::StartStopMenu)
            .def("AddDialogToRender", &CDialogHolder::AddDialogToRender)
            .def("RemoveDialogToRender", &CDialogHolder::RemoveDialogToRender)
    ];
});

SCRIPT_EXPORT(CUIDialogWnd, (CDialogHolder), {
    module(luaState)
    [
        class_<CUIDialogWnd, CUIWindow>("CUIDialogWnd")
            .def("ShowDialog", &CUIDialogWnd::ShowDialog)
            .def("HideDialog", &CUIDialogWnd::HideDialog)
            .def("GetHolder", &CUIDialogWnd::GetHolder)
            .def("SetHolder", &CUIDialogWnd::SetHolder)
    ];
});

SCRIPT_EXPORT(CUIMessageBoxEx, (CUIDialogWnd), {
    module(luaState)
    [
        class_<CUIMessageBoxEx, CUIDialogWnd>("CUIMessageBoxEx")
            .def(constructor<>())
            .def("InitMessageBox", &CUIMessageBoxEx::InitMessageBox)
            .def("SetText", &CUIMessageBoxEx::SetText)
            .def("GetHost", &CUIMessageBoxEx::GetHost)
            .def("GetPassword", &CUIMessageBoxEx::GetPassword)
    ];
});

SCRIPT_EXPORT(CUIMMShniaga, (CUIWindow), {
    module(luaState)
    [
        class_<CUIMMShniaga, CUIWindow>("CUIMMShniaga")
        .enum_("enum_page_id")
        [
            value("epi_main", CUIMMShniaga::epi_main),
            value("epi_new_game", CUIMMShniaga::epi_new_game),
            value("epi_new_network_game", CUIMMShniaga::epi_new_network_game)
        ]
        .def("SetVisibleMagnifier", &CUIMMShniaga::SetVisibleMagnifier)
        .def("SetPage", &CUIMMShniaga::SetPage)
        .def("ShowPage", &CUIMMShniaga::ShowPage)
    ];
});

SCRIPT_EXPORT(CUISleepStatic, (CUIStatic),
    { module(luaState)[class_<CUISleepStatic, CUIStatic>("CUISleepStatic").def(constructor<>())]; });

SCRIPT_EXPORT(SServerFilters, (), {
    module(luaState)
    [
        class_<SServerFilters>("SServerFilters")
            .def(constructor<>())
            .def_readwrite("empty", &SServerFilters::empty)
            .def_readwrite("full", &SServerFilters::full)
            .def_readwrite("with_pass", &SServerFilters::with_pass)
            .def_readwrite("without_pass", &SServerFilters::without_pass)
            .def_readwrite("without_ff", &SServerFilters::without_ff)
            .def_readwrite("listen_servers", &SServerFilters::listen_servers)
    ];
});

SCRIPT_EXPORT(connect_error_cb, (), {
    module(luaState)
    [
        class_<connect_error_cb>("connect_error_cb")
            .def(constructor<>())
            .def(constructor<connect_error_cb::lua_object_type, connect_error_cb::lua_function_type>())
            .def("bind", (connect_error_cb::lua_bind_type)(&connect_error_cb::bind))
            .def("clear", &connect_error_cb::clear)
    ];
});

SCRIPT_EXPORT(CServerList, (CUIWindow), {
    module(luaState)
    [
        class_<CServerList, CUIWindow>("CServerList")
            .def(constructor<>())
            .enum_("enum_connect_errcode")
            [
                value("ece_unique_nick_not_registred", int(ece_unique_nick_not_registred)),
                value("ece_unique_nick_expired", int(ece_unique_nick_expired))
            ]
            .def("SetConnectionErrCb", &CServerList::SetConnectionErrCb)
            .def("ConnectToSelected", &CServerList::ConnectToSelected)
            .def("SetFilters", &CServerList::SetFilters)
            .def("SetPlayerName", &CServerList::SetPlayerName)
            .def("RefreshList", &CServerList::RefreshGameSpyList)
            .def("RefreshQuick", &CServerList::RefreshQuick)
            .def("ShowServerInfo", &CServerList::ShowServerInfo)
            .def("NetRadioChanged", &CServerList::NetRadioChanged)
            .def("SetSortFunc", &CServerList::SetSortFunc)
    ];
});

SCRIPT_EXPORT(CUIMapList, (CUIWindow), {
    module(luaState)[class_<CUIMapList, CUIWindow>("CUIMapList")
                         .def(constructor<>())
                         .def("SetWeatherSelector", &CUIMapList::SetWeatherSelector)
                         .def("SetModeSelector", &CUIMapList::SetModeSelector)
                         .def("OnModeChange", &CUIMapList::OnModeChange)
                         .def("LoadMapList", &CUIMapList::LoadMapList)
                         .def("SaveMapList", &CUIMapList::SaveMapList)
                         .def("GetCommandLine", &CUIMapList::GetCommandLine)
                         .def("SetServerParams", &CUIMapList::SetServerParams)
                         .def("GetCurGameType", &CUIMapList::GetCurGameType)
                         .def("StartDedicatedServer", &CUIMapList::StartDedicatedServer)
                         .def("SetMapPic", &CUIMapList::SetMapPic)
                         .def("SetMapInfo", &CUIMapList::SetMapInfo)
                         .def("ClearList", &CUIMapList::ClearList)
                         .def("IsEmpty", &CUIMapList::IsEmpty)];
});

SCRIPT_EXPORT(EnumGameIDs, (), {
    class EnumGameIDs
    {
    };
    module(luaState)
    [
        class_<EnumGameIDs>("GAME_TYPE")
            .enum_("gametype")
            [
                value("GAME_UNKNOWN", int(-1)),
                             value("eGameIDDeathmatch", int(eGameIDDeathmatch)),
                             value("eGameIDTeamDeathmatch", int(eGameIDTeamDeathmatch)),
                             value("eGameIDArtefactHunt", int(eGameIDArtefactHunt)),
                             value("eGameIDCaptureTheArtefact", int(eGameIDCaptureTheArtefact))
            ]
    ];
});
// clang-format on
