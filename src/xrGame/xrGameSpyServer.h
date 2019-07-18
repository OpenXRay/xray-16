#pragma once

#include "xrServer.h"
#include "xrGameSpy/GameSpy_GCD_Server.h"
#include "xrGameSpy/GameSpy_QR2.h"

class xrGameSpyClientData : public xrClientData
{
private:
    typedef xrClientData inherited;

public:
    string64 m_pChallengeString;
    int m_iCDKeyReauthHint;
    bool m_bCDKeyAuth;
    u32 suspiciousActionCount;

    xrGameSpyClientData();
    virtual void Clear();
    virtual ~xrGameSpyClientData();
};

class xrGameSpyServer : public xrServer
{
private:
    typedef xrServer inherited;

    enum
    {
        server_flag_password = u8(1 << 0),
        server_flag_protected = u8(1 << 1),
        server_flag_2 = u8(1 << 2),
        server_flag_3 = u8(1 << 3),
        server_flag_4 = u8(1 << 4),
        server_flag_5 = u8(1 << 5),
        server_flag_6 = u8(1 << 6),
        server_flag_128 = u8(1 << 7),
        server_flag_all = u8(-1)
    };

private:
    int m_iReportToMasterServer;

    BOOL m_bQR2_Initialized;
    void QR2_Init(int PortID);
    void QR2_ShutDown();

    BOOL m_bCDKey_Initialized;
    void CDKey_Init();
    void CDKey_ShutDown();
    void SendChallengeString_2_Client(IClient* C);

    CGameSpy_GCD_Server m_GCDServer;
    CGameSpy_QR2 m_QR2;
    int iGameSpyBasePort;

protected:
    virtual bool NeedToCheckClient_GameSpy_CDKey(IClient* CL);
    virtual bool Check_ServerAccess(IClient* CL, string512& reason);

public:
    shared_str HostName;
    shared_str MapName;
    shared_str Password;
    Flags8 ServerFlags;

    int m_iMaxPlayers;
    bool m_bCheckCDKey;

    int GetPlayersCount();
    void xr_stdcall OnCDKey_Validation(int LocalID, int res, char* errormsg);
    void xr_stdcall OnCDKey_ReValidation(int LocalID, int hint, char* challenge);
    CGameSpy_QR2* QR2() { return &m_QR2; };
    CGameSpy_GCD_Server* GCD_Server() { return &m_GCDServer; }
    virtual bool HasPassword();
    virtual bool HasProtected();

    virtual void Assign_ServerType(string512& res);
    virtual void GetServerInfo(CServerInfo* si);
    bool IsPublicServer() const { return m_iReportToMasterServer != 0; };
public:
    xrGameSpyServer();
    virtual ~xrGameSpyServer();

    virtual EConnect Connect(shared_str& session_name, GameDescriptionData& game_descr);
    virtual void Update();

    //	virtual void			OnCL_Connected		(IClient* C);
    virtual void OnCL_Disconnected(IClient* C);
    virtual IClient* client_Create();

    virtual u32 OnMessage(
        NET_Packet& P, ClientID /*DPNID*/ sender); // Non-Zero means broadcasting with "flags" as returned
    virtual void OnError_Add(qr2_error_t error){};
};
