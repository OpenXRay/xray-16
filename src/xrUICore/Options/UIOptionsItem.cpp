#include "pch.hpp"
#include "UIOptionsItem.h"
#include "UIOptionsManager.h"
#include "xrEngine/XR_IOConsole.h"

CUIOptionsManager CUIOptionsItem::m_optionsManager;

CUIOptionsItem::CUIOptionsItem() : m_dep(sdNothing) {}
CUIOptionsItem::~CUIOptionsItem() { m_optionsManager.UnRegisterItem(this); }

void CUIOptionsItem::AssignProps(const shared_str& entry, const shared_str& group)
{
    m_optionsManager.RegisterItem(this, group);
    m_entry = entry;
}

void CUIOptionsItem::SendMessage2Group(pcstr group, pcstr message)
{
    m_optionsManager.SendMessage2Group(group, message);
}

void CUIOptionsItem::OnMessage(LPCSTR message)
{
    // do nothing
}

pcstr CUIOptionsItem::GetOptStringValue() const
{
    return Console->GetString(m_entry.c_str());
}

void CUIOptionsItem::SaveOptStringValue(LPCSTR val) const
{
    xr_string command = m_entry.c_str();
    command += " ";
    command += val;
    Console->Execute(command.c_str());
}

void CUIOptionsItem::GetOptIntegerValue(int& val, int& min, int& max) const
{
    val = Console->GetInteger(m_entry.c_str(), min, max);
}

void CUIOptionsItem::SaveOptIntegerValue(int val) const
{
    string512 command;
    xr_sprintf(command, "%s %d", m_entry.c_str(), val);
    Console->Execute(command);
}

void CUIOptionsItem::GetOptFloatValue(float& val, float& min, float& max) const
{
    val = Console->GetFloat(m_entry.c_str(), min, max);
}

void CUIOptionsItem::SaveOptFloatValue(float val) const
{
    string512 command;
    xr_sprintf(command, "%s %f", m_entry.c_str(), val);
    Console->Execute(command);
}

bool CUIOptionsItem::GetOptBoolValue() const
{
    return Console->GetBool(m_entry.c_str());
}

void CUIOptionsItem::SaveOptBoolValue(bool val) const
{
    string512 command;
    xr_sprintf(command, "%s %s", m_entry.c_str(), (val) ? "1" : "0");
    Console->Execute(command);
}

pcstr CUIOptionsItem::GetOptTokenValue() const
{
    return Console->GetToken(m_entry.c_str());
}

const xr_token* CUIOptionsItem::GetOptToken() const
{
    return Console->GetXRToken(m_entry.c_str());
}

Fvector3 CUIOptionsItem::GetOptVector3Value() const
{
    return Console->GetFVector(m_entry.c_str());
}

void CUIOptionsItem::SaveOptVector3Value(Fvector3 val) const
{
    string512 command;
    xr_sprintf(command, "%s %f %f %f", m_entry.c_str(), val.x, val.y, val.z);
    Console->Execute(command);
}

Fvector4 CUIOptionsItem::GetOptVector4Value() const
{
    return Console->GetFVector4(m_entry.c_str());
}

void CUIOptionsItem::SaveOptVector4Value(Fvector4 val) const
{
    string512 command;
    xr_sprintf(command, "%s %f %f %f %f", m_entry.c_str(), val.x, val.y, val.z, val.w);
    Console->Execute(command);
}

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
