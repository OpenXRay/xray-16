#include "stdafx.h"
#include "Level.h"
#include "xrCore/stream_reader.h"
#include "MainMenu.h"
#include "string_table.h"
#include "xrEngine/xr_ioconsole.h"

bool CLevel::synchronize_map_data()
{
    deny_m_spawn = FALSE;
    map_data.m_map_sync_received = true;
    return synchronize_client();
}

bool CLevel::synchronize_client()
{
    Server->OnCL_Connected(Server->GetServerClient());

    if (game_configured)
    {
        deny_m_spawn = FALSE;
        return true;
    }

    if (Server)
    {
        ClientReceive();
        Server->Update();
    }

    return !!game_configured;
}
