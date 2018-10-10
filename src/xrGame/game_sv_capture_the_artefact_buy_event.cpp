#include "StdAfx.h"
#include "game_sv_capture_the_artefact.h"

void game_sv_CaptureTheArtefact::OnPlayerBuyFinished(ClientID id_who, NET_Packet& P)
{
    game_PlayerState* ps = get_id(id_who);
    VERIFY2(ps, make_string("player state not found (ClientID = 0x%08x)", id_who.value()).c_str());
    CSE_ALifeCreatureActor* e_Actor = smart_cast<CSE_ALifeCreatureActor*>(get_entity_from_eid(ps->GameID));
    VERIFY2(e_Actor || ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD),
        make_string("server entity of actor not found (GameID = 0x%08x)", ps->GameID).c_str());

    DestroyAllPlayerItems(id_who);
    ClearPlayerItems(ps);

    s32 moneyDif;
    P.r_s32(moneyDif);

    u16 itemsCount = 0;
    P.r_u16(itemsCount);

    ps->LastBuyAcount = moneyDif;

    for (u16 i = 0; i != itemsCount; ++i)
    {
        u8 tempGroupId;
        u8 tempItemId;
        P.r_u8(tempGroupId);
        P.r_u8(tempItemId);
        ps->pItemList.push_back((tempGroupId << 8) | tempItemId);
    }

    if (ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
    {
        TGameIDToBoughtFlag::iterator buyer_iter = m_dead_buyers.find(id_who);
        if (buyer_iter != m_dead_buyers.end())
            buyer_iter->second = 1;
        else
            m_dead_buyers.insert(std::make_pair(id_who, 1));
    }
    else
    {
        SpawnWeaponsForActor(e_Actor, ps);
    }
    SetCanOpenBuyMenu(id_who);
}

void game_sv_CaptureTheArtefact::OnCloseBuyMenuFromAll() { m_buyMenuPlayerStates.clear(); }
void game_sv_CaptureTheArtefact::OnPlayerCloseBuyMenu(xrClientData const* pclient)
{
    R_ASSERT2(pclient, "bad client closed buy menu");
    TBuyMenuPlayerStates::iterator temp_iter = m_buyMenuPlayerStates.find(pclient);
    if (temp_iter != m_buyMenuPlayerStates.end())
    {
        game_PlayerState* ps = temp_iter->first->ps;
        if (!ps)
            return;
        VERIFY2(!ps->testFlag(GAME_PLAYER_FLAG_SPECTATOR), "spectator can't send OnPlayerCloseBuyMenu messages");
        if (ps->testFlag(GAME_PLAYER_FLAG_SPECTATOR))
            return;
        VERIFY2(
            ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD), "alife players can't send OnPlayerCloseBuyMenu messages");
        if (!ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
            return;

#ifdef DEBUG
        Msg("--- Player [%s] closes buy menu", pclient->ps->getName());
#endif // #ifdef DEBUG
        if (temp_iter->second == buyMenuPlayerReadyToSpawn)
        {
            RespawnClient(pclient);
        }
        temp_iter->second = buyMenuPlayerClosesBuyMenu;
    }
}

void game_sv_CaptureTheArtefact::OnPlayerOpenBuyMenu(xrClientData const* pclient)
{
    R_ASSERT(pclient->ps);
    if (!pclient->ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
        return;
// R_ASSERT2(pclient->ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD), "only dead players can open buy menu in async
// mode");
#ifdef DEBUG
    Msg("--- Player [%s] opens buy menu", pclient->ps->getName());
#endif // #ifdef DEBUG
    m_buyMenuPlayerStates.insert(std::make_pair(pclient, buyMenuPlayerOpensBuyMenu));
}

bool game_sv_CaptureTheArtefact::CheckIfPlayerInBuyMenu(xrClientData const* pclient)
{
    TBuyMenuPlayerStates::const_iterator temp_iter = m_buyMenuPlayerStates.find(pclient);
    if (temp_iter != m_buyMenuPlayerStates.end())
    {
        return ((temp_iter->second == buyMenuPlayerOpensBuyMenu) || (temp_iter->second == buyMenuPlayerReadyToSpawn));
    }
    return false;
}
void game_sv_CaptureTheArtefact::SetReadyToSpawnPlayer(xrClientData const* pclient)
{
    R_ASSERT(pclient->ps);
// if (!pclient->ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
//	return;
// R_ASSERT2(pclient->ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD), "only dead players can open buy menu in async
// mode");
#ifdef DEBUG
    Msg("--- Player [%s] opens buy menu", pclient->ps->getName());
#endif // #ifdef DEBUG
    m_buyMenuPlayerStates.insert(std::make_pair(pclient, buyMenuPlayerReadyToSpawn));
}

bool game_sv_CaptureTheArtefact::CanChargeFreeAmmo(char const* ammo_section)
{
    VERIFY2(m_not_free_ammo_str.size(), "'not_free_ammo' not initialized");
    if (!m_not_free_ammo_str.size())
        return true;
    if (!ammo_section)
        return true;

    if (!xr_strlen(ammo_section))
        return true;

    if (strstr(m_not_free_ammo_str.c_str(), ammo_section))
        return false;

    return true;
}
