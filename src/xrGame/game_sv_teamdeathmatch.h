#pragma once

#include "game_sv_deathmatch.h"

class game_sv_TeamDeathmatch : public game_sv_Deathmatch
{
private:
    typedef game_sv_Deathmatch inherited;
    bool teams_swaped;

protected:
    virtual bool checkForFragLimit();
    virtual bool HasChampion();

    virtual void ReadOptions(shared_str& options);
    virtual void ConsoleCommands_Create();
    virtual void ConsoleCommands_Clear();

public:
    game_sv_TeamDeathmatch() : teams_swaped(false) { m_type = eGameIDTeamDeathmatch; }
    virtual void Create(shared_str& options);

    virtual void OnEvent(NET_Packet& tNetPacket, u16 type, u32 time, ClientID sender);

    virtual LPCSTR type_name() const { return "teamdeathmatch"; };
    virtual void Update();
    virtual void net_Export_State(NET_Packet& P, ClientID id_to); // full state
    // Events
    virtual void OnPlayerConnect(ClientID id_who);
    virtual void OnPlayerConnectFinished(ClientID id_who);

    virtual void OnPlayerSelectTeam(NET_Packet& P, ClientID sender);
    virtual void OnPlayerChangeTeam(ClientID id_who, s16 team);
    virtual KILL_RES GetKillResult(game_PlayerState* pKiller, game_PlayerState* pVictim);
    virtual bool OnKillResult(KILL_RES KillResult, game_PlayerState* pKiller, game_PlayerState* pVictim);
    virtual void OnPlayerKillPlayer(game_PlayerState* ps_killer, game_PlayerState* ps_killed, KILL_TYPE KillType,
        SPECIAL_KILL_TYPE SpecialKillType, CSE_Abstract* pWeaponA);
    virtual void UpdateTeamScore(game_PlayerState* ps_killer, s16 OldKills);
    virtual bool CheckTeams() { return true; };
    virtual void OnPlayerHitPlayer(u16 id_hitter, u16 id_hitted, NET_Packet& P);
    virtual void OnPlayerHitPlayer_Case(game_PlayerState* ps_hitter, game_PlayerState* ps_hitted, SHit* pHitS);

    virtual void OnRoundStart();
    virtual void OnRoundEnd();
    virtual void AutoBalanceTeams();
    virtual void AutoSwapTeams();

    virtual u8 AutoTeam();
    virtual u32 GetPlayersCountInTeams(u8 team);
    virtual bool TeamSizeEqual();
    virtual u32 RP_2_Use(CSE_Abstract* E);

    virtual void LoadTeams();

    pcstr GetAnomalySetBaseName() override { return "teamdeathmatch_game_anomaly_sets"; };
    virtual BOOL CanHaveFriendlyFire() { return TRUE; }
    virtual void OnFraglimitExceed();
    virtual void OnTimelimitExceed();

    virtual BOOL isFriendlyFireEnabled();
    virtual float GetFriendlyFire();

    virtual BOOL Get_AutoTeamBalance();
    virtual BOOL Get_AutoTeamSwap();
    virtual BOOL Get_FriendlyIndicators();
    virtual BOOL Get_FriendlyNames();

    virtual int Get_TeamKillLimit();
    virtual BOOL Get_TeamKillPunishment();

    BOOL OnTouchItem(CSE_ActorMP* actor, CSE_Abstract* item);
    void OnDetachItem(CSE_ActorMP* actor, CSE_Abstract* item);

    virtual BOOL OnTouch(u16 eid_who, u16 eid_what, BOOL bForced = FALSE);
    virtual void OnDetach(u16 eid_who, u16 eid_what);

    void OnObjectEnterTeamBase(u16 id, u16 zone_team);
    void OnObjectLeaveTeamBase(u16 id, u16 zone_team);
    virtual void RespawnPlayer(ClientID id_who, bool NoSpectator);

protected:
    virtual void WriteGameState(CInifile& ini, LPCSTR sect, bool bRoundResult);
};
