#pragma once

#include "xrUICore/Static/UIStatic.h"

class CUIXml;
class CUIGameLog;
struct KillMessageStruct;

class CUIMoneyIndicator final : public CUIWindow
{
public:
    CUIMoneyIndicator();
    void InitFromXML(CUIXml& xml_doc);
    void SetMoneyAmount(pcstr money);
    void SetMoneyChange(pcstr money);
    void AddBonusMoney(KillMessageStruct& msg);
    pcstr GetDebugType() override { return "CUIMoneyIndicator"; }

protected:
    CUIStatic m_back{ "Background" };
    CUIStatic m_money_amount{ "Money amount" };
    CUIStatic m_money_change{ "Money change" };
    CUIGameLog* m_pBonusMoney;
};
