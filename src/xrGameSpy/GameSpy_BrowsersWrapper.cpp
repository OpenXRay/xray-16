#include "stdafx.h"
#include "GameSpy_BrowsersWrapper.h"

CGameSpy_BrowsersWrapper::CGameSpy_BrowsersWrapper()
{
    char secretKey[16];
    FillSecretKey(secretKey);
    CGameSpy_Browser::SMasterListConfig cfg = {GAMESPY_GAMENAME, secretKey};
    browser = xr_make_unique<CGameSpy_Browser>(cfg);
}

bool CGameSpy_BrowsersWrapper::Init(UpdateCallback updateCb) { return browser->Init(updateCb); }

void CGameSpy_BrowsersWrapper::Clear() { browser->Clear(); }

GSUpdateStatus CGameSpy_BrowsersWrapper::RefreshList_Full(bool Local, const char* FilterStr)
{
    return browser->RefreshList_Full(Local, FilterStr);
}

void CGameSpy_BrowsersWrapper::RefreshQuick(int Index) { browser->RefreshQuick(Index); }
bool CGameSpy_BrowsersWrapper::HasAllKeys(int Index) { return browser->HasAllKeys(Index); }
bool CGameSpy_BrowsersWrapper::CheckDirectConnection(int Index) { return browser->CheckDirectConnection(Index); }

int CGameSpy_BrowsersWrapper::GetServersCount() { return browser->GetServersCount(); }
void CGameSpy_BrowsersWrapper::GetServerInfoByIndex(ServerInfo* pServerInfo, int idx)
{
    return browser->GetServerInfoByIndex(pServerInfo, idx);
}

void* CGameSpy_BrowsersWrapper::GetServerByIndex(int id) { return browser->GetServerByIndex(id); }

bool CGameSpy_BrowsersWrapper::GetBool(void* srv, int keyId, bool defaultValue)
{
    return browser->GetBool(srv, keyId, defaultValue);
}
int CGameSpy_BrowsersWrapper::GetInt(void* srv, int keyId, int defaultValue)
{
    return browser->GetInt(srv, keyId, defaultValue);
}
float CGameSpy_BrowsersWrapper::GetFloat(void* srv, int keyId, float defaultValue)
{
    return browser->GetFloat(srv, keyId, defaultValue);
}

GSUpdateStatus CGameSpy_BrowsersWrapper::Update() { return browser->Update(); }
