#include "pch.hpp"
#include "UIOptionsManager.h"
#include "UIOptionsItem.h"
#include "xrEngine/XR_IOConsole.h"

CUIOptionsManager::CUIOptionsManager() : m_restart_flags(0) {}
void CUIOptionsManager::RegisterItem(CUIOptionsItem* item, const shared_str& group)
{
    groups_it it = m_groups.find(group);

    if (m_groups.end() != it)
    {
        (*it).second.push_back(item);
    }
    else
    {
        group_name gr_name = group;
        items_list list;

        list.push_back(item);
        m_groups.insert(std::make_pair(gr_name, list));
    }
}

void CUIOptionsManager::UnRegisterItem(CUIOptionsItem* item)
{
    groups_it it;
    for (it = m_groups.begin(); it != m_groups.end(); it++)
    {
        for (u32 i = 0; i < (*it).second.size(); i++)
            if ((*it).second[i] == item)
            {
                (*it).second.erase((*it).second.begin() + i);
                return;
            }
    }
}

void CUIOptionsManager::SendMessage2Group(const shared_str& group, const char* message)
{
    groups_it it = m_groups.find(group);

    R_ASSERT2(m_groups.end() != it, "invalid group name");

    for (u32 i = 0; i < (*it).second.size(); i++)
        (*it).second[i]->OnMessage(message);
}

void CUIOptionsManager::SaveBackupValues(const shared_str& group)
{
    groups_it it = m_groups.find(group);

    R_ASSERT3(m_groups.end() != it, "invalid group name", group.c_str());

    for (u32 i = 0; i < (*it).second.size(); i++)
    {
        (*it).second[i]->SaveBackUpOptValue();
    }
}

void CUIOptionsManager::SetCurrentValues(const shared_str& group)
{
    groups_it it = m_groups.find(group);

    R_ASSERT3(m_groups.end() != it, "invalid group name", group.c_str());

    for (u32 i = 0; i < (*it).second.size(); i++)
        (*it).second[i]->SetCurrentOptValue();
}

void CUIOptionsManager::SaveValues(const shared_str& group)
{
    groups_it it = m_groups.find(group);

    R_ASSERT3(m_groups.end() != it, "invalid group name", group.c_str());

    for (u32 i = 0; i < (*it).second.size(); i++)
    {
        CUIOptionsItem* oi = (*it).second[i];
        if (oi->IsChangedOptValue())
            oi->SaveOptValue();
    }
}

void CUIOptionsManager::UndoGroup(const shared_str& group)
{
    groups_it it = m_groups.find(group);
    R_ASSERT2(m_groups.end() != it, "invalid group name");

    for (u32 i = 0; i < (*it).second.size(); i++)
    {
        CUIOptionsItem* oi = (*it).second[i];
        if (oi->IsChangedOptValue())
            oi->UndoOptValue();
    }
}

void CUIOptionsManager::OptionsPostAccept()
{
    if (m_restart_flags & e_vid_restart)
        Console->Execute("vid_restart");

    if (m_restart_flags & e_snd_restart)
        Console->Execute("snd_restart");

    if (m_restart_flags & e_ui_restart)
        Console->Execute("ui_restart");

    m_restart_flags &= ~e_vid_restart;
    m_restart_flags &= ~e_snd_restart;
    m_restart_flags &= ~e_ui_restart;
}

void CUIOptionsManager::DoVidRestart() { m_restart_flags |= e_vid_restart; }
void CUIOptionsManager::DoSndRestart() { m_restart_flags |= e_snd_restart; }
void CUIOptionsManager::DoUIRestart() { m_restart_flags |= e_ui_restart; }
void CUIOptionsManager::DoSystemRestart() { m_restart_flags |= e_system_restart; }
