#pragma once
#include "xrGameSpy/xrGameSpy.h"
#include "xrCore/Threading/Lock.hpp"
#include "xrCore/_std_extensions.h"

class CServerList;
class CGameSpy_QR2;

struct GameInfo
{
    shared_str InfoName;
    shared_str InfoData;
    GameInfo(LPCSTR Name, LPCSTR Data)
    {
        InfoName._set(Name);
        InfoData._set(Data);
    };
};

struct PlayerInfo
{
    string128 Name;
    s16 Frags;
    u16 Deaths;
    u8 Rank;
    u8 Team;
    bool Spectator;
    u8 Artefacts;
};

struct TeamInfo
{
    u8 Score;
};

struct ServerInfo
{
    //	SBServer pSBServer;
    string128 m_Address;
    string128 m_HostName;
    string128 m_ServerName;
    string128 m_SessionName;
    string128 m_ServerGameType;
    string128 m_ServerVersion;
    u32 m_GameType;

    s16 m_ServerNumPlayers;
    s16 m_ServerMaxPlayers;
    string128 m_ServerUpTime;
    s16 m_ServerNumTeams;
    bool m_bDedicated;
    bool m_bFFire;
    s16 m_s16FFire;
    bool m_bPassword;
    bool m_bUserPass;
    s16 m_Ping;
    s16 m_Port, m_HPort;
    int MaxPing;
    bool MapRotation;
    bool VotingEnabled;
    int SpectratorModes;
    int FragLimit;
    float TimeLimit;
    float DamageBlockTime;
    bool DamageBlockIndicators;
    bool AnomaliesEnabled;
    float AnomaliesTime;
    float ForceRespawn;
    float WarmUp;
    bool AutoTeamBalance;
    bool AutoTeamSwap;
    bool FriendlyIndicators;
    bool FriendlyNames;
    float FriendlyFire;
    int ArtefactCount;
    float ArtefactStayTime;
    float ArtefactRespawnTime;
    float Reinforcement;
    bool ShieldedBases;
    bool ReturnPlayers;
    bool BearerCantSprint;

    xr_vector<PlayerInfo> m_aPlayers;
    xr_vector<TeamInfo> m_aTeams;

    int Index;

    ServerInfo(){};
    ServerInfo(string128 NewAddress) { xr_strcpy(m_Address, NewAddress); };
    bool operator==(LPCSTR Address)
    {
        int res = xr_strcmp(m_Address, Address);
        return res == 0;
    };
};

enum class GSUpdateStatus
{
    Success,
    ConnectingToMaster,
    MasterUnreachable,
    OutOfService,
    Unknown
};

struct _ServerBrowser;

class XRGAMESPY_API CGameSpy_Browser
{
public:
    using UpdateCallback = fastdelegate::FastDelegate<void(CGameSpy_Browser*)>;

    struct SMasterListConfig
    {
        pcstr gamename;
        pcstr secretkey;
    };

private:
    //	string16	m_SecretKey;
    _ServerBrowser* m_pGSBrowser;
    CGameSpy_QR2* m_pQR2;
    UpdateCallback onUpdate;

    void ReadServerInfo(ServerInfo* pServerInfo, void* pServer);

    bool m_bAbleToConnectToMasterServer;
    bool m_bTryingToConnectToMasterServer;
    bool m_bShowCMSErr;
    bool m_inited;

    Lock m_refresh_lock;

public:
    CGameSpy_Browser(const SMasterListConfig& masterListCfg);
    ~CGameSpy_Browser();

    bool Init(UpdateCallback updateCb);
    void Clear();

    GSUpdateStatus RefreshList_Full(bool Local, const char* FilterStr = "");
    void RefreshQuick(int Index);
    bool HasAllKeys(int Index);
    bool CheckDirectConnection(int Index);

    void CallBack_OnUpdateCompleted();

    int GetServersCount();
    void GetServerInfoByIndex(ServerInfo* pServerInfo, int idx);
    void* GetServerByIndex(int id);
    bool GetBool(void* srv, int keyId, bool defaultValue = false);
    int GetInt(void* srv, int keyId, int defaultValue = 0);
    float GetFloat(void* srv, int keyId, float defaultValue = 0.0f);
    const char* GetString(void* srv, int keyId, const char* defaultValue = nullptr);

    void OnUpdateFailed(void* server);
    GSUpdateStatus Update();

    void UpdateServerList();
    void SortBrowserByPing();

    void RefreshListInternet(const char* FilterStr);
};
