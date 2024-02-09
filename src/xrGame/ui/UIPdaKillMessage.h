#pragma once

#include "KillMessageStruct.h"
#include "xrUICore/Static/UIStatic.h"

class CUIPdaKillMessage final : public CUIColorAnimConrollerContainer
{
    using inherited = CUIColorAnimConrollerContainer;

public:
    CUIPdaKillMessage();

    void Init(KillMessageStruct& msg, CGameFont* F);

    pcstr GetDebugType() override { return "CUIPdaKillMessage"; }

protected:
    float InitText(CUIStatic& refStatic, float x, ColoredName& info);
    float InitIcon(CUIStatic& refStatic, float x, IconInfo& info);

    CUIStatic m_victim_name{ "Victim name" };
    CUIStatic m_initiator  { "Initiator" };
    CUIStatic m_killer_name{ "Killer name" };
    CUIStatic m_ext_info   { "Ext. info" };
};
