////////////////////////////////////////////////////////////////////////////
// Module : XR_IOConsole_get.cpp
// Created : 17.05.2008
// Author : Evgeniy Sokolov
// Description : Console`s get-functions class implementation
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XR_IOConsole.h"
#include "xr_ioc_cmd.h"

bool CConsole::GetBool(LPCSTR cmd) const
{
    IConsole_Command* cc = GetCommand(cmd);
    CCC_Mask* cf = dynamic_cast<CCC_Mask*>(cc);
    if (cf)
    {
        return (cf->GetValue() != 0);
    }

    CCC_Integer* ci = dynamic_cast<CCC_Integer*>(cc);
    if (ci)
    {
        return (ci->GetValue() != 0);
    }
    return false;
}

float CConsole::GetFloat(LPCSTR cmd, float& min, float& max) const
{
    min = 0.0f;
    max = 0.0f;
    IConsole_Command* cc = GetCommand(cmd);
    CCC_Float* cf = dynamic_cast<CCC_Float*>(cc);
    if (cf)
    {
        cf->GetBounds(min, max);
        return cf->GetValue();
    }
    return 0.0f;
}

IConsole_Command* CConsole::GetCommand(LPCSTR cmd) const
{
    vecCMD_CIT it = Commands.find(cmd);
    if (it == Commands.end())
        return NULL;
    else
        return it->second;
}

int CConsole::GetInteger(LPCSTR cmd, int& min, int& max) const
{
    min = 0;
    max = 1;
    IConsole_Command* cc = GetCommand(cmd);

    CCC_Integer* cf = dynamic_cast<CCC_Integer*>(cc);
    if (cf)
    {
        cf->GetBounds(min, max);
        return cf->GetValue();
    }
    CCC_Mask* cm = dynamic_cast<CCC_Mask*>(cc);
    if (cm)
    {
        min = 0;
        max = 1;
        return (cm->GetValue()) ? 1 : 0;
    }
    return 0;
}

LPCSTR CConsole::GetString(LPCSTR cmd) const
{
    IConsole_Command* cc = GetCommand(cmd);
    if (!cc)
        return NULL;

    static IConsole_Command::TStatus stat;
    cc->GetStatus(stat);
    return stat;
}

LPCSTR CConsole::GetToken(LPCSTR cmd) const { return GetString(cmd); }
const xr_token* CConsole::GetXRToken(LPCSTR cmd) const
{
    IConsole_Command* cc = GetCommand(cmd);

    CCC_Token* cf = dynamic_cast<CCC_Token*>(cc);
    if (cf)
    {
        return cf->GetToken();
    }
    return NULL;
}

Fvector* CConsole::GetFVectorPtr(LPCSTR cmd) const
{
    IConsole_Command* cc = GetCommand(cmd);
    CCC_Vector3* cf = dynamic_cast<CCC_Vector3*>(cc);
    if (cf)
    {
        return cf->GetValuePtr();
    }
    return NULL;
}

Fvector CConsole::GetFVector(LPCSTR cmd) const
{
    Fvector* pV = GetFVectorPtr(cmd);
    if (pV)
    {
        return *pV;
    }
    return Fvector().set(0.0f, 0.0f, 0.0f);
}
