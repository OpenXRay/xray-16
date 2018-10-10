////////////////////////////////////////////////////////////////////////////
//	Module 		: autosave_manager.cpp
//	Created 	: 04.11.2004
//  Modified 	: 04.11.2004
//	Author		: Dmitriy Iassenev
//	Description : Autosave manager
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "autosave_manager.h"
#include "date_time.h"
#include "ai_space.h"
#include "Level.h"
#include "xrMessages.h"
#include "UIGameCustom.h"
#include "Actor.h"
#include "MainMenu.h"
#include "xrNetServer/NET_Messages.h"

extern LPCSTR alife_section;

CAutosaveManager::CAutosaveManager()
{
    u32 hours, minutes, seconds;
    LPCSTR section = alife_section;

    sscanf(pSettings->r_string(section, "autosave_interval"), "%d:%d:%d", &hours, &minutes, &seconds);
    m_autosave_interval = (u32)generate_time(1, 1, 1, hours, minutes, seconds);
    m_last_autosave_time = Device.dwTimeGlobal;

    sscanf(pSettings->r_string(section, "delay_autosave_interval"), "%d:%d:%d", &hours, &minutes, &seconds);
    m_delay_autosave_interval = (u32)generate_time(1, 1, 1, hours, minutes, seconds);

    m_not_ready_count = 0;

    shedule.t_min = 5000;
    shedule.t_max = 5000;
    shedule_register();
}

CAutosaveManager::~CAutosaveManager() { shedule_unregister(); }
float CAutosaveManager::shedule_Scale() { return (.5f); }
void CAutosaveManager::shedule_Update(u32 dt)
{
    inherited::shedule_Update(dt);
#pragma todo("Plecha to Plecha : AUTOSAVE (do not forgive to enable it in release version:-))))!!!!")
    if (true)
        return;
    if (!ai().get_alife())
        return;

    if (last_autosave_time() + autosave_interval() >= Device.dwTimeGlobal)
        return;

    if (Device.dwPrecacheFrame || !g_actor || !ready_for_autosave() || !Actor()->g_Alive())
    {
        delay_autosave();
        return;
    }

    update_autosave_time();

    string_path temp;
    strconcat(sizeof(temp), temp, Core.UserName, " - ", "autosave");
    NET_Packet net_packet;
    net_packet.w_begin(M_SAVE_GAME);
    net_packet.w_stringZ(temp);
    net_packet.w_u8(0);
    Level().Send(net_packet, net_flags(TRUE));

    string_path S1;
    xr_strcat(temp, sizeof(temp), ".dds");
    FS.update_path(S1, "$game_saves$", temp);

    MainMenu()->Screenshot(IRender::SM_FOR_GAMESAVE, S1);
#ifdef WINDOWS
    SetFileAttributes(S1, FILE_ATTRIBUTE_HIDDEN);
#endif
    CurrentGameUI()->AddCustomStatic("autosave", true);
}

void CAutosaveManager::on_game_loaded() { m_last_autosave_time = Device.dwTimeGlobal; }
