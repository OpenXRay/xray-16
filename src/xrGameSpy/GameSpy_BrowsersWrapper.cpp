#include "stdafx.h"
#include "GameSpy_BrowsersWrapper.h"
#include "xrCommon/xr_array.h"
#include "xrCore/Threading/ScopeLock.hpp"

CGSUpdateStatusAccumulator::CGSUpdateStatusAccumulator(GSUpdateStatus s) { Register(s); }

void CGSUpdateStatusAccumulator::Register(GSUpdateStatus s) { accumulator.push_back(s); }

void CGSUpdateStatusAccumulator::Reset(GSUpdateStatus s)
{
    ScopeLock sl(&lock);
    accumulator.clear();
    Register(s);
}

GSUpdateStatus CGSUpdateStatusAccumulator::GetOptimistic() const
{
    ScopeLock sl(&lock);
    GSUpdateStatus res = GSUpdateStatus::Unknown;
    for (auto status : accumulator)
    {
        if (status < res)
        {
            res = status;
            if (res == GSUpdateStatus::Success)
                break;
        }
    }
    return res;
}

GSUpdateStatus CGSUpdateStatusAccumulator::GetPessimistic() const
{
    ScopeLock sl(&lock);
    GSUpdateStatus res = GSUpdateStatus::Success;
    for (auto status : accumulator)
    {
        if (status > res)
        {
            res = status;
            if (res == GSUpdateStatus::Unknown)
                break;
        }
    }
    return res;
}

bool CGSUpdateStatusAccumulator::IsStatusGood(GSUpdateStatus s) { return (s <= GSUpdateStatus::ConnectingToMaster); }

struct SBrowserConfig
{
    CGameSpy_Browser::SMasterListConfig cfg;
    bool active;
};

static constexpr SBrowserConfig cop_master_bro = { {GAMESPY_GAMENAME, GAMESPY_GAMEKEY}, true};
static constexpr SBrowserConfig cs_master_bro  = { {GAMESPY_GAMENAME_CS, GAMESPY_GAMEKEY_CS}, true};
static constexpr SBrowserConfig soc_master_bro = { {GAMESPY_GAMENAME_SOC, GAMESPY_GAMEKEY_SOC}, true};

static constexpr xr_array<SBrowserConfig, 3> master_lists = {cop_master_bro, cs_master_bro, soc_master_bro};

CGameSpy_BrowsersWrapper::CGameSpy_BrowsersWrapper()
{
    browsers.resize(master_lists.size());
    for (size_t i = 0; i < master_lists.size(); ++i)
    {
        browsers[i].browser = xr_make_unique<CGameSpy_Browser>(master_lists[i].cfg);
        browsers[i].active = master_lists[i].active;
        browsers[i].reportedFailure = false;
        browsers[i].servers_count = 0;

        CGameSpy_Browser::UpdateCallback cb;
        cb.bind(this, &CGameSpy_BrowsersWrapper::UpdateCb);
        auto res = browsers[i].browser->Init(cb);
        R_ASSERT(res);
    }
}

CGameSpy_BrowsersWrapper::~CGameSpy_BrowsersWrapper()
{
    for (auto& bro : browsers)
    {
        bro.browser->Clear();
    };
}

CGameSpy_BrowsersWrapper::SubscriberIdx CGameSpy_BrowsersWrapper::SubscribeUpdates(UpdateCallback updateCb)
{
    ScopeLock sl(&updates_subscriptions_lock);
    for (auto i = 0; i < updates_subscriptions.size(); ++i)
    {
        if (updates_subscriptions[i].empty())
        {
            updates_subscriptions[i] = updateCb;
            return i;
        }
    }

    updates_subscriptions.push_back(updateCb);
    return updates_subscriptions.size() - 1;
}

bool CGameSpy_BrowsersWrapper::UnsubscribeUpdates(SubscriberIdx idx)
{
    ScopeLock sl(&updates_subscriptions_lock);
    bool res = !updates_subscriptions[idx].empty();
    if (res)
        updates_subscriptions[idx].clear();

    return res;
}

void CGameSpy_BrowsersWrapper::UpdateCb(CGameSpy_Browser* gs_browser)
{
    {
        ScopeLock sl1(&servers_lock);

        SBrowserInfo* bro_info = nullptr;
        for (auto& bro : browsers)
        {
            if (bro.browser.get() == gs_browser)
            {
                bro_info = &bro;
                break;
            }
        }
        R_ASSERT(bro_info);

        auto cur_count = gs_browser->GetServersCount();
        R_ASSERT(cur_count >= bro_info->servers_count);

        // Such mechanism of addition to list (with intermediate vector) prevents possible problems with re-ordering
        // servers after investigating the new ones. I'm sure that changing indices will be unexpected in client's code.
        while (cur_count > bro_info->servers_count)
        {
            servers.push_back({gs_browser, bro_info->servers_count, nullptr});
            ++bro_info->servers_count;
        }
    }
    {
        ScopeLock sl2(&updates_subscriptions_lock);
        for (auto i = 0; i < updates_subscriptions.size(); ++i)
        {
            if (!updates_subscriptions[i].empty())
                updates_subscriptions[i]();
        }
    }
}

void CGameSpy_BrowsersWrapper::ForgetAllServers()
{
    ScopeLock sl(&servers_lock);
    servers.clear();
    for (auto& bro : browsers)
    {
        bro.servers_count = 0;
        bro.reportedFailure = false;
    }
}

GSUpdateStatus CGameSpy_BrowsersWrapper::RefreshList_Full(bool Local, const char* FilterStr)
{
    CGSUpdateStatusAccumulator acc(GSUpdateStatus::OutOfService);

    ScopeLock sl(&servers_lock);
    ForgetAllServers();

    for (auto& bro : browsers)
    {
        if (!bro.active)
            continue;

        auto status = bro.browser->RefreshList_Full(Local, FilterStr);
        acc.Register(status);
        if (!CGSUpdateStatusAccumulator::IsStatusGood(status))
            bro.reportedFailure = true;
    };

    return acc.GetOptimistic();
}

void CGameSpy_BrowsersWrapper::RefreshQuick(int server_id)
{
    ScopeLock sl(&servers_lock);
    R_ASSERT(server_id < servers.size());
    servers[server_id].browser->RefreshQuick(servers[server_id].idx);
}

bool CGameSpy_BrowsersWrapper::HasAllKeys(int server_id)
{
    ScopeLock sl(&servers_lock);
    R_ASSERT(server_id < servers.size());
    return servers[server_id].browser->HasAllKeys(servers[server_id].idx);
}

bool CGameSpy_BrowsersWrapper::CheckDirectConnection(int server_id)
{
    ScopeLock sl(&servers_lock);
    R_ASSERT(server_id < servers.size());
    return servers[server_id].browser->CheckDirectConnection(servers[server_id].idx);
}

int CGameSpy_BrowsersWrapper::GetServersCount()
{
    ScopeLock sl(&servers_lock);
    return servers.size();
}

void CGameSpy_BrowsersWrapper::GetServerInfoByIndex(ServerInfo* pServerInfo, int server_id)
{
    ScopeLock sl(&servers_lock);
    R_ASSERT(server_id < servers.size());
    servers[server_id].browser->GetServerInfoByIndex(pServerInfo, servers[server_id].idx);

    // Correct server ID from 'local' (from the actual browser) to 'global' (from the unified proxy)
    pServerInfo->Index = server_id;
}

void* CGameSpy_BrowsersWrapper::GetServerByIndex(int server_id)
{
    ScopeLock sl(&servers_lock);
    R_ASSERT(server_id < servers.size());
    servers[server_id].gs_data = servers[server_id].browser->GetServerByIndex(servers[server_id].idx);

    // Returning "raw" gs_data is not the best solution - it forces us to iterate over all servers in the vector (when
    // calling the Get* methods) to determine which browser we should use. However, returning a pointer to
    // SServerDescription from the vector is much worse - count of the servers grows while update is performing,
    // so the pointer will become invalid after the vector is re-allocated.
    return servers[server_id].gs_data;
}

bool CGameSpy_BrowsersWrapper::GetBool(void* srv, int keyId, bool defaultValue)
{
    ScopeLock sl(&servers_lock);
    for (auto& server : servers)
    {
        if (server.gs_data == srv)
            return server.browser->GetBool(srv, keyId, defaultValue);
    }
    R_ASSERT(false);
    return defaultValue;
}
int CGameSpy_BrowsersWrapper::GetInt(void* srv, int keyId, int defaultValue)
{
    ScopeLock sl(&servers_lock);
    for (auto& server : servers)
    {
        if (server.gs_data == srv)
            return server.browser->GetInt(srv, keyId, defaultValue);
    }
    R_ASSERT(false);
    return defaultValue;
}
float CGameSpy_BrowsersWrapper::GetFloat(void* srv, int keyId, float defaultValue)
{
    ScopeLock sl(&servers_lock);
    for (auto& server : servers)
    {
        if (server.gs_data == srv)
            return server.browser->GetFloat(srv, keyId, defaultValue);
    }
    R_ASSERT(false);
    return defaultValue;
}

GSUpdateStatus CGameSpy_BrowsersWrapper::Update()
{
    CGSUpdateStatusAccumulator acc(GSUpdateStatus::MasterUnreachable);
    ScopeLock sl(&servers_lock);

    size_t nonWorkBroCnt = 0;
    for (auto& bro : browsers)
    {
        auto status = bro.browser->Update();
        acc.Register(status);
        if (!CGSUpdateStatusAccumulator::IsStatusGood(status))
            bro.reportedFailure = true;
        if (bro.reportedFailure || !bro.active)
            ++nonWorkBroCnt;
    }

    if (nonWorkBroCnt < browsers.size())
    {
        return acc.GetOptimistic();
    }
    else
    {
        ForgetAllServers();
        return acc.GetPessimistic();
    }
}
