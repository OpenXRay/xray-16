#include "StdAfx.h"
#include "xrGameSpyServer.h"
#include "xrMessages.h"
#include "gamespy/GameSpy_QR2_callbacks.h"

/////////////////////// QR2 ///////////////////////////////////////
// void			xrGameSpyServer::QR2_Init			(u32 PortID)
void xrGameSpyServer::QR2_Init(int PortID)
{
    CGameSpy_QR2::SInitConfig ctx;
    ctx.OnServerKey = callback_serverkey;
    ctx.OnPlayerKey = callback_playerkey;
    ctx.OnTeamKey = callback_teamkey;
    ctx.OnKeyList = callback_keylist;
    ctx.OnCount = callback_count;
    ctx.OnError = callback_adderror;
    ctx.OnNatNeg = callback_nn;
    ctx.OnClientMessage = callback_cm;
    ctx.OnDenyIP = callback_deny_ip;
    ctx.GSServer = this;
    if (!m_QR2.Init(PortID, m_iReportToMasterServer, ctx))
        return;
    m_bQR2_Initialized = TRUE;
};

void xrGameSpyServer::QR2_ShutDown()
{
    m_bQR2_Initialized = FALSE;
    m_QR2.ShutDown(NULL);
};

//------------------------------- CD_Key -----------------------------

void xrGameSpyServer::CDKey_Init()
{
    if (!m_GCDServer.Init())
        return;
    m_bCDKey_Initialized = TRUE;
};

void xrGameSpyServer::CDKey_ShutDown()
{
    m_GCDServer.ShutDown();
    m_bCDKey_Initialized = FALSE;
};

// generate a rand nchar challenge

void xrGameSpyServer::SendChallengeString_2_Client(IClient* C)
{
    if (!C)
        return;
    xrGameSpyClientData* pClient = (xrGameSpyClientData*)C;

    m_GCDServer.CreateRandomChallenge(pClient->m_pChallengeString, 8);
    //--------- Send Respond ---------------------------------------------
    NET_Packet P;

    P.w_begin(M_GAMESPY_CDKEY_VALIDATION_CHALLENGE);
    P.w_u8(0);
    P.w_stringZ(pClient->m_pChallengeString);
    SendTo(pClient->ID, P);
}

void xrGameSpyServer::OnCDKey_Validation(int LocalID, int res, char* errormsg)
{
    ClientID ID;
    ID.set(u32(LocalID));
    xrGameSpyClientData* CL = (xrGameSpyClientData*)ID_to_client(ID);
    if (0 != res)
    {
        CL->m_bCDKeyAuth = true;
#ifndef MASTER_GOLD
        Msg("xrGS::CDKey: Validation successful - <%s>", errormsg);
#endif // #ifndef MASTER_GOLD
        Check_GameSpy_CDKey_Success(CL);
    }
    else
    {
        Msg("CDKey: Validation failed - <%s>", errormsg);
        SendConnectResult(CL, u8(res), ecr_cdkey_validation_failed, errormsg);
    }
};

void xrGameSpyServer::OnCDKey_ReValidation(int LocalID, int hint, char* challenge)
{
    ClientID ID;
    ID.set(u32(LocalID));
    xrGameSpyClientData* CL = (xrGameSpyClientData*)ID_to_client(ID);
    if (!CL)
        return;
    xr_strcpy(CL->m_pChallengeString, challenge);
    CL->m_iCDKeyReauthHint = hint;
    //--------- Send Respond ---------------------------------------------
    NET_Packet P;
    P.w_begin(M_GAMESPY_CDKEY_VALIDATION_CHALLENGE);
    P.w_u8(1);
    P.w_stringZ(CL->m_pChallengeString);
    SendTo(CL->ID, P);
}
