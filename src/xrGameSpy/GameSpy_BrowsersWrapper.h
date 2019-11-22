#pragma once
#include "xrGameSpy/xrGameSpy.h"

enum class GSUpdateStatus;
class CGameSpy_Browser;
struct ServerInfo;

class XRGAMESPY_API CGameSpy_BrowsersWrapper
{
public:
    using UpdateCallback = fastdelegate::FastDelegate<void()>;
    using DestroyCallback = fastdelegate::FastDelegate<void(CGameSpy_BrowsersWrapper*)>;

private:
    xr_unique_ptr<CGameSpy_Browser> browser;

public:
    CGameSpy_BrowsersWrapper();

    bool Init(UpdateCallback updateCb);
    void Clear();

    GSUpdateStatus RefreshList_Full(bool Local, const char* FilterStr = "");
    void RefreshQuick(int Index);
    bool HasAllKeys(int Index);
    bool CheckDirectConnection(int Index);

    int GetServersCount();
    void GetServerInfoByIndex(ServerInfo* pServerInfo, int idx);
    void* GetServerByIndex(int id);
    bool GetBool(void* srv, int keyId, bool defaultValue = false);
    int GetInt(void* srv, int keyId, int defaultValue = 0);
    float GetFloat(void* srv, int keyId, float defaultValue = 0.0f);

    GSUpdateStatus Update();

    /*
    //Unused???
    void CallBack_OnUpdateCompleted();
    void UpdateServerList();
    void SortBrowserByPing();
    void OnUpdateFailed(void* server);
    const char* GetString(void* srv, int keyId, const char* defaultValue = nullptr);

    void RefreshListInternet(const char* FilterStr);*/
};
