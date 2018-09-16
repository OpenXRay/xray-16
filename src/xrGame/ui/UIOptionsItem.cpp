#include "StdAfx.h"
#include "UIOptionsItem.h"
#include "UIOptionsManager.h"
#include "xrEngine/xr_ioconsole.h"

CUIOptionsManager CUIOptionsItem::m_optionsManager;

CUIOptionsItem::CUIOptionsItem() : m_dep(sdNothing) {}
CUIOptionsItem::~CUIOptionsItem() { m_optionsManager.UnRegisterItem(this); }
void CUIOptionsItem::AssignProps(const shared_str& entry, const shared_str& group)
{
    m_optionsManager.RegisterItem(this, group);
    m_entry = entry;
}

void CUIOptionsItem::SendMessage2Group(LPCSTR group, LPCSTR message)
{
    m_optionsManager.SendMessage2Group(group, message);
}

void CUIOptionsItem::OnMessage(LPCSTR message)
{
    // do nothing
}

LPCSTR CUIOptionsItem::GetOptStringValue() { return Console->GetString(m_entry.c_str()); }
void CUIOptionsItem::SaveOptStringValue(LPCSTR val)
{
    xr_string command = m_entry.c_str();
    command += " ";
    command += val;
    Console->Execute(command.c_str());
}

void CUIOptionsItem::GetOptIntegerValue(int& val, int& min, int& max)
{
    val = Console->GetInteger(m_entry.c_str(), min, max);
}

void CUIOptionsItem::SaveOptIntegerValue(int val)
{
    string512 command;
    xr_sprintf(command, "%s %d", m_entry.c_str(), val);
    Console->Execute(command);
}

void CUIOptionsItem::GetOptFloatValue(float& val, float& min, float& max)
{
    val = Console->GetFloat(m_entry.c_str(), min, max);
}

void CUIOptionsItem::SaveOptFloatValue(float val)
{
    string512 command;
    xr_sprintf(command, "%s %f", m_entry.c_str(), val);
    Console->Execute(command);
}

bool CUIOptionsItem::GetOptBoolValue() { return Console->GetBool(m_entry.c_str()); }
void CUIOptionsItem::SaveOptBoolValue(bool val)
{
    string512 command;
    xr_sprintf(command, "%s %s", m_entry.c_str(), (val) ? "1" : "0");
    Console->Execute(command);
}

LPCSTR CUIOptionsItem::GetOptTokenValue() { return Console->GetToken(m_entry.c_str()); }
const xr_token* CUIOptionsItem::GetOptToken() { return Console->GetXRToken(m_entry.c_str()); }
void CUIOptionsItem::SaveOptValue()
{
    if (!IsChangedOptValue())
        return;

    if (m_dep == sdVidRestart)
        m_optionsManager.DoVidRestart();
    else if (m_dep == sdSndRestart)
        m_optionsManager.DoSndRestart();
    else if (m_dep == sdUIRestart)
        m_optionsManager.DoUIRestart();
    else if (m_dep == sdSystemRestart)
        m_optionsManager.DoSystemRestart();
}

void CUIOptionsItem::OnChangedOptValue()
{
    if (m_dep == sdApplyOnChange)
        SaveOptValue();
}

void CUIOptionsItem::UndoOptValue()
{
    if (m_dep == sdApplyOnChange)
        SaveOptValue();
}
