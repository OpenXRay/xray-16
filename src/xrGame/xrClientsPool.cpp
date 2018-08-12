#include "StdAfx.h"
#include "xrClientsPool.h"
#include "xrServer.h"

extern u32 g_sv_Client_Reconnect_Time;

xrClientsPool::xrClientsPool() { m_dclients.reserve(MAX_PLAYERS_COUNT); }
xrClientsPool::~xrClientsPool() { Clear(); }
void xrClientsPool::Clear()
{
    for (dclients_t::iterator i = m_dclients.begin(), ie = m_dclients.end(); i != ie; ++i)
    {
        xr_delete(i->m_client);
    }
    m_dclients.clear();
}

bool const xrClientsPool::expired_client_deleter::operator()(dclient& right) const
{
    if ((m_current_time - right.m_dtime) > m_expire_time)
    {
        xr_delete(right.m_client);
        return true;
    }
    return false;
}

void xrClientsPool::ClearExpiredClients()
{
    expired_client_deleter tmp_deleter;
    tmp_deleter.m_current_time = Device.dwTimeGlobal;
    tmp_deleter.m_expire_time = g_sv_Client_Reconnect_Time * 60 * 1000; // in minutes
    m_dclients.erase(std::remove_if(m_dclients.begin(), m_dclients.end(), tmp_deleter), m_dclients.end());
}

void xrClientsPool::Add(xrClientData* new_dclient)
{
    if (!new_dclient->ps)
        return;

    if (g_sv_Client_Reconnect_Time == 0)
    {
        xr_delete(new_dclient);
        return;
    }

    dclient tmp_dclient;
    tmp_dclient.m_client = new_dclient;
    tmp_dclient.m_dtime = Device.dwTimeGlobal;
    m_dclients.push_back(tmp_dclient);
}

bool const xrClientsPool::pooled_client_finder::operator()(dclient const& right) const
{
    if (!right.m_client->ps || !m_new_client->ps)
        return false;

    if (m_new_client->m_cdkey_digest != right.m_client->m_cdkey_digest)
        return false;

    if (!xr_strcmp(m_new_client->ps->getName(), right.m_client->ps->getName()))
        return true;

    return false;
}

xrClientData* xrClientsPool::Get(xrClientData* new_client)
{
    ClearExpiredClients();
    pooled_client_finder tmp_finder;
    tmp_finder.m_new_client = new_client;
    dclients_t::iterator tmp_iter = std::find_if(m_dclients.begin(), m_dclients.end(), tmp_finder);
    if (tmp_iter != m_dclients.end())
    {
        xrClientData* result = tmp_iter->m_client;
        m_dclients.erase(tmp_iter);
        return result;
    }
    return NULL;
}
