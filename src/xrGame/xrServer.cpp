// xrServer.cpp: implementation of the xrServer class.
//
//////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "xrServer.h"
#include "xrMessages.h"
#include "xrServer_Objects_ALife_All.h"
#include "Level.h"
#include "game_cl_base.h"
#include "game_sv_base.h"
#include "ai_space.h"
#include "xrEngine/IGame_Persistent.h"
#include "string_table.h"
#include "Common/object_broker.h"
#include "xrEngine/Engine.h"
#include "xrEngine/GameFont.h"
#include "xrEngine/PerformanceAlert.hpp"
#include "xrEngine/XR_IOConsole.h"
#include "ui/UIInventoryUtilities.h"
#include <functional>

#pragma warning(push)
#pragma warning(disable : 4995)
#include <malloc.h>
#pragma warning(pop)

xrClientData::xrClientData() : IClient()
{
    Clear();
}

void xrClientData::Clear()
{
    owner = NULL;
    net_Accepted = FALSE;
};

xrClientData::~xrClientData() { }

xrServer::xrServer() : IPureServer(Device.GetTimerGlobal(), GEnv.isDedicatedServer)
{
    m_last_update_time = 0;
}

xrServer::~xrServer()
{
    client_Destroy(SV_Client);
    entities.clear();
    SV_Client = nullptr;
}

//--------------------------------------------------------------------

CSE_Abstract* xrServer::ID_to_entity(u16 ID)
{
    // #pragma todo("??? to all : ID_to_entity - must be replaced to 'game->entity_from_eid()'")
    if (0xffff == ID)
        return 0;
    xrS_entities::iterator I = entities.find(ID);
    if (entities.end() != I)
        return I->second;
    else
        return 0;
}

//--------------------------------------------------------------------
void xrServer::client_Destroy(IClient* C)
{
    if (SV_Client)
    {
        CSE_Abstract* pOwner = static_cast<xrClientData*>(SV_Client)->owner;
        if (pOwner)
        {
            game->CleanDelayedEventFor(pOwner->ID);
        }
    }
}

//--------------------------------------------------------------------
int g_Dump_Update_Write = 0;

#ifdef DEBUG
INT g_sv_SendUpdate = 0;
#endif

void xrServer::Update()
{
#ifdef DEBUG
    VERIFY(verify_entities());
#endif

    // game update
    game->ProcessDelayedEvent();
    game->Update();

    if (game->sv_force_sync)
        Perform_game_export();

#ifdef DEBUG
    VERIFY(verify_entities());
#endif
}

u32 xrServer::OnMessage(NET_Packet& P, ClientID sender) // Non-Zero means broadcasting with "flags" as returned
{
    u16 type;
    P.r_begin(type);

#ifdef DEBUG
    VERIFY(verify_entities());
#endif
    xrClientData* CL = ID_to_client(sender);

    switch (type)
    {
    case M_UPDATE:
    {
        Process_update(P, sender); // No broadcast
#ifdef DEBUG
        VERIFY(verify_entities());
#endif
    }
    break;
    case M_SPAWN:
    {
        if (SV_Client)
            Process_spawn(P, sender);

#ifdef DEBUG
        VERIFY(verify_entities());
#endif
    }
    break;
    case M_EVENT:
    {
        Process_event(P, sender);
#ifdef DEBUG
        VERIFY(verify_entities());
#endif
    }
    break;
    case M_EVENT_PACK:
    {
        NET_Packet tmpP;
        while (!P.r_eof())
        {
            tmpP.B.count = P.r_u8();
            P.r(&tmpP.B.data, tmpP.B.count);

            OnMessage(tmpP, sender);
        };
    }
    break;
    case M_SWITCH_DISTANCE:
    {
        game->switch_distance(P, sender);
#ifdef DEBUG
        VERIFY(verify_entities());
#endif
    }
    break;
    case M_CHANGE_LEVEL:
    {
        if (game->change_level(P, sender))
        {
            SendBroadcast(BroadcastCID, P);
        }
#ifdef DEBUG
        VERIFY(verify_entities());
#endif
    }
    break;
    case M_SAVE_GAME:
    {
        game->save_game(P, sender);
#ifdef DEBUG
        VERIFY(verify_entities());
#endif
    }
    break;
    case M_LOAD_GAME:
    {
        game->load_game(P, sender);
        SendBroadcast(BroadcastCID, P);
#ifdef DEBUG
        VERIFY(verify_entities());
#endif
    }
    break;
    case M_RELOAD_GAME:
    {
        SendBroadcast(BroadcastCID, P);
#ifdef DEBUG
        VERIFY(verify_entities());
#endif
    }
    break;
    case M_SAVE_PACKET:
    {
        Process_save(P, sender);
#ifdef DEBUG
        VERIFY(verify_entities());
#endif
    }break;
    case M_CHANGE_LEVEL_GAME:
    {
        ClientID CID; CID.set(0xffffffff);
        SendBroadcast(CID, P);
    }break;
	case M_PLAYER_FIRE:
        break;
    }

#ifdef DEBUG
    VERIFY(verify_entities());
#endif

    return 0;
}

void xrServer::SendTo(ClientID ID, NET_Packet& P)
{
    Level().OnMessage(P.B.data, P.B.count);
}

void xrServer::SendBroadcast(ClientID exclude, NET_Packet& P)
{
    if (!SV_Client)
        return;
    if (SV_Client->ID == exclude)
        return;

    xrClientData* CL = static_cast<xrClientData*>(SV_Client);
    if (!CL->net_Accepted)
        return;

    SendTo(SV_Client->ID, P);
}
//--------------------------------------------------------------------
CSE_Abstract* xrServer::entity_Create(pcstr name) { return F_entity_Create(name); }
void xrServer::entity_Destroy(CSE_Abstract*& P)
{
#ifdef DEBUG
    if (dbg_net_Draw_Flags.test(dbg_destroy))
        Msg("xrServer::entity_Destroy : [%d][%s][%s]", P->ID, P->name(), P->name_replace());
#endif
    R_ASSERT(P);
    entities.erase(P->ID);
    m_tID_Generator.vfFreeID(P->ID, Device.TimerAsync());

    if (P->owner && P->owner->owner == P)
        P->owner->owner = NULL;

    P->owner = NULL;
    if (!ai().get_alife() || !P->m_bALifeControl)
    {
        F_entity_Destroy(P);
    }
}

CSE_Abstract* xrServer::GetEntity(u32 Num)
{
    xrS_entities::iterator I = entities.begin(), E = entities.end();
    for (u32 C = 0; I != E; ++I, ++C)
    {
        if (C == Num)
            return I->second;
    };
    return NULL;
};

#ifdef DEBUG

static BOOL _ve_initialized = FALSE;
static BOOL _ve_use = TRUE;

bool xrServer::verify_entities() const
{
    if (!_ve_initialized)
    {
        _ve_initialized = TRUE;
        if (strstr(Core.Params, "-~ve"))
            _ve_use = FALSE;
    }
    if (!_ve_use)
        return true;

    xrS_entities::const_iterator I = entities.begin();
    xrS_entities::const_iterator E = entities.end();
    for (; I != E; ++I)
    {
        VERIFY2((*I).first != 0xffff, "SERVER : Invalid entity id as a map key - 0xffff");
        VERIFY2((*I).second, "SERVER : Null entity object in the map");
        VERIFY3((*I).first == (*I).second->ID,
            "SERVER : ID mismatch - map key doesn't correspond to the real entity ID", (*I).second->name_replace());
        verify_entity((*I).second);
    }
    return (true);
}

void xrServer::verify_entity(const CSE_Abstract* entity) const
{
    VERIFY(entity->m_wVersion != 0);
    if (entity->ID_Parent != 0xffff)
    {
        xrS_entities::const_iterator J = entities.find(entity->ID_Parent);
        VERIFY2(J != entities.end(),
            make_string("SERVER : Cannot find parent in the map [%s][%s]", entity->name_replace(), entity->name())
                .c_str());
        VERIFY3((*J).second, "SERVER : Null entity object in the map", entity->name_replace());
        VERIFY3((*J).first == (*J).second->ID,
            "SERVER : ID mismatch - map key doesn't correspond to the real entity ID", (*J).second->name_replace());
        VERIFY3(std::find((*J).second->children.begin(), (*J).second->children.end(), entity->ID) !=
                (*J).second->children.end(),
            "SERVER : Parent/Children relationship mismatch - Object has parent, but corresponding parent doesn't have "
            "children",
            (*J).second->name_replace());
    }

    xr_vector<u16>::const_iterator I = entity->children.begin();
    xr_vector<u16>::const_iterator E = entity->children.end();
    for (; I != E; ++I)
    {
        VERIFY3(*I != 0xffff, "SERVER : Invalid entity children id - 0xffff", entity->name_replace());
        xrS_entities::const_iterator J = entities.find(*I);
        VERIFY3(J != entities.end(), "SERVER : Cannot find children in the map", entity->name_replace());
        VERIFY3((*J).second, "SERVER : Null entity object in the map", entity->name_replace());
        VERIFY3((*J).first == (*J).second->ID,
            "SERVER : ID mismatch - map key doesn't correspond to the real entity ID", (*J).second->name_replace());
        VERIFY3((*J).second->ID_Parent == entity->ID,
            "SERVER : Parent/Children relationship mismatch - Object has children, but children doesn't have parent",
            (*J).second->name_replace());
    }
}

#endif // DEBUG

shared_str xrServer::level_name(const shared_str& server_options) const { return (game->level_name(server_options)); }
shared_str xrServer::level_version(const shared_str& server_options) const
{
    return (game_sv_GameState::parse_level_version(server_options));
}

void xrServer::CreateSVClient()
{
    SV_Client = new xrClientData();
    SV_Client->ID.set(1);
}

void xrServer::GetServerInfo( CServerInfo* si )
{
    string32 tmp;
    string256 tmp256;

    LPCSTR time =
        InventoryUtilities::GetTimeAsString(Device.dwTimeGlobal, InventoryUtilities::etpTimeToSecondsAndDay).c_str();
    si->AddItem("Uptime", time, RGB(255, 228, 0));

	xr_strcpy(tmp256, "single");
	
	{
		xr_strcat(tmp256, " time limit [");
		xr_strcat(tmp256, itoa( 0, tmp, 10 ));
		xr_strcat(tmp256, "] ");
	}

	si->AddItem("Game type", tmp256, RGB(128,255,255));

	if (g_pGameLevel)
	{
		time = InventoryUtilities::GetGameTimeAsString(InventoryUtilities::etpTimeToMinutes).c_str();
		
		xr_strcpy(tmp256, time);

		si->AddItem("Game time", tmp256, RGB(205,228,178));
	}
}
