#include "stdafx.h"
#include "GameSpy_QR2.h"
#include "GameSpy_Keys.h"

namespace
{
qr2_error_t xrGS_qr2_initA(qr2_t* qrec, const gsi_char* ip, int baseport, int ispublic, int natnegotiate,
    qr2_serverkeycallback_t server_key_callback, qr2_playerteamkeycallback_t player_key_callback,
    qr2_playerteamkeycallback_t team_key_callback, qr2_keylistcallback_t key_list_callback,
    qr2_countcallback_t playerteam_count_callback, qr2_adderrorcallback_t adderror_callback, void* userdata)
{
    int BasePort = baseport;
    if (BasePort == -1)
        BasePort = GAMESPY_QR2_BASEPORT;
    else
    {
        if (BasePort < START_PORT)
            BasePort = START_PORT;
        if (BasePort > END_PORT)
            BasePort = END_PORT;
    }

    qr2_error_t res = qr2_initA(qrec, ip, BasePort, GAMESPY_GAMENAME, GAMESPY_GAMEKEY, ispublic, natnegotiate,
        server_key_callback, player_key_callback, team_key_callback, key_list_callback, playerteam_count_callback,
        adderror_callback, userdata);

    return res;
}
}

void CGameSpy_QR2::Think(void* qrec) { qr2_think(static_cast<qr2_t>(qrec)); }
void CGameSpy_QR2::ShutDown(void* qrec) { qr2_shutdown(static_cast<qr2_t>(qrec)); }
void CGameSpy_QR2::RegisterAdditionalKeys()
{
    //---- Global Keys ----
    qr2_register_keyA(GAMETYPE_NAME_KEY, ("gametypename"));
    qr2_register_keyA(DEDICATED_KEY, ("dedicated"));
    //---- game_sv_base ---
    qr2_register_keyA(G_MAP_ROTATION_KEY, ("maprotation"));
    qr2_register_keyA(G_VOTING_ENABLED_KEY, ("voting"));
    //---- game sv mp ----
    qr2_register_keyA(G_SPECTATOR_MODES_KEY, ("spectatormodes"));
    qr2_register_keyA(G_MAX_PING_KEY, ("max_ping_limit"));
    qr2_register_keyA(G_USER_PASSWORD_KEY, ("user_password"));

    //---- game_sv_deathmatch ----
    qr2_register_keyA(G_FRAG_LIMIT_KEY, ("fraglimit"));
    qr2_register_keyA(G_TIME_LIMIT_KEY, ("timelimit"));
    qr2_register_keyA(G_DAMAGE_BLOCK_TIME_KEY, ("damageblocktime"));
    qr2_register_keyA(G_DAMAGE_BLOCK_INDICATOR_KEY, ("damageblockindicator"));
    qr2_register_keyA(G_ANOMALIES_ENABLED_KEY, ("anomalies"));
    qr2_register_keyA(G_ANOMALIES_TIME_KEY, ("anomaliestime"));
    qr2_register_keyA(G_WARM_UP_TIME_KEY, ("warmuptime"));
    qr2_register_keyA(G_FORCE_RESPAWN_KEY, ("forcerespawn"));
    //---- game_sv_teamdeathmatch ----
    qr2_register_keyA(G_AUTO_TEAM_BALANCE_KEY, ("autoteambalance"));
    qr2_register_keyA(G_AUTO_TEAM_SWAP_KEY, ("autoteamswap"));
    qr2_register_keyA(G_FRIENDLY_INDICATORS_KEY, ("friendlyindicators"));
    qr2_register_keyA(G_FRIENDLY_NAMES_KEY, ("friendlynames"));
    qr2_register_keyA(G_FRIENDLY_FIRE_KEY, ("friendlyfire"));
    //---- game_sv_artefacthunt ----
    qr2_register_keyA(G_ARTEFACTS_COUNT_KEY, ("artefactscount"));
    qr2_register_keyA(G_ARTEFACT_STAY_TIME_KEY, ("artefactstaytime"));
    qr2_register_keyA(G_ARTEFACT_RESPAWN_TIME_KEY, ("artefactrespawntime"));
    qr2_register_keyA(G_REINFORCEMENT_KEY, ("reinforcement"));
    qr2_register_keyA(G_SHIELDED_BASES_KEY, ("shieldedbases"));
    qr2_register_keyA(G_RETURN_PLAYERS_KEY, ("returnplayers"));
    qr2_register_keyA(G_BEARER_CANT_SPRINT_KEY, ("bearercant_sprint"));

    //---- Player keys
    //	qr2_register_keyA(P_NAME__KEY,					("name_"));
    //	qr2_register_keyA(P_FRAGS__KEY,					("frags_"));
    //	qr2_register_keyA(P_DEATH__KEY,					("death_"));
    //	qr2_register_keyA(P_RANK__KEY,					("rank_"));
    //	qr2_register_keyA(P_TEAM__KEY,					("p_team_"));
    qr2_register_keyA(P_SPECTATOR__KEY, ("spectator_"));
    qr2_register_keyA(P_ARTEFACTS__KEY, ("artefacts_"));

    //---- Team keys
    //	qr2_register_keyA(T_NAME_KEY,					("t_name_key"));
    qr2_register_keyA(T_SCORE_T_KEY, ("t_score_t"));
    qr2_register_keyA(SERVER_UP_TIME_KEY, ("server_up_time"));
};

bool CGameSpy_QR2::Init(int PortID, int Public, SInitConfig& ctx)
{        
    qr2_error_t err = xrGS_qr2_initA(nullptr, 
                                     nullptr, 
                                     PortID, 
                                     Public, 
                                     0, 
                                     ctx.OnServerKey, 
                                     ctx.OnPlayerKey, 
                                     ctx.OnTeamKey,
                                     ctx.OnKeyList, 
                                     ctx.OnCount, 
                                     ctx.OnError, 
                                     ctx.GSServer);
        
#ifndef MASTER_GOLD
    Msg("xrGS::xrGS_qr2_initA returned code is [%d]", err);
#endif // #ifndef MASTER_GOLD

    if (err != e_qrnoerror)
    {
        Msg("xrGS::QR2 : Failes to Initialize!");
        return false;
    }

    RegisterAdditionalKeys();

    // Set a function to be called when we receive a game specific message
    qr2_register_clientmessage_callback(NULL, ctx.OnClientMessage);

    // Set a function to be called when we receive a nat negotiation request
    qr2_register_natneg_callback(NULL, ctx.OnNatNeg);

    // Set a function to be called when gamespy responds my IP and port number    
    qr2_register_denyresponsetoip_callback(NULL, ctx.OnDenyIP);

#ifndef MASTER_GOLD
    Msg("xrGS::QR2 : Initialized");
#endif // #ifndef MASTER_GOLD
    return true;
};

void CGameSpy_QR2::BufferAdd(void* outbuf, const char* value)
{
    qr2_buffer_addA(static_cast<qr2_buffer_t>(outbuf), value);
};

void CGameSpy_QR2::BufferAdd_Int(void* outbuf, int value)
{
    qr2_buffer_add_int(static_cast<qr2_buffer_t>(outbuf), value);
};

void CGameSpy_QR2::KeyBufferAdd(void* keybuffer, int keyid)
{
    qr2_keybuffer_add(static_cast<qr2_keybuffer_t>(keybuffer), keyid);
}

const char* CGameSpy_QR2::GetGameVersion() { return GAME_VERSION; }
const char* CGameSpy_QR2::RegisteredKey(u32 KeyID) { return qr2_registered_key_list[KeyID]; }
