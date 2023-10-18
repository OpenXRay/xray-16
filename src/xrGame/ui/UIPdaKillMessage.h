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
    float InitText(CUITextWnd& refStatic, float x, ColoredName& info);
    float InitIcon(CUIStatic& refStatic, float x, IconInfo& info);

    CUITextWnd m_victim_name;
    CUIStatic m_initiator{ "Initiator" };
    CUITextWnd m_killer_name;
    CUIStatic m_ext_info{ "Ext. info" };
};
