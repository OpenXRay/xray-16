#pragma once

#include "xrUICore/Static/UIStatic.h"
#include "KillMessageStruct.h"

class CUIXml;
class CUIGameLog;

class CUIMoneyIndicator final : public CUIWindow
{
public:
    CUIMoneyIndicator();
    virtual ~CUIMoneyIndicator();
    virtual void Update();
    void InitFromXML(CUIXml& xml_doc);
    void SetMoneyAmount(LPCSTR money);
    void SetMoneyChange(LPCSTR money);
    void AddBonusMoney(KillMessageStruct& msg);
    pcstr GetDebugType() override { return "CUIMoneyIndicator"; }

protected:
    CUIStatic m_back{ "Background" };
    CUIStatic m_money_amount{ "Money amount" };
    CUIStatic m_money_change{ "Money change" };
    CUIGameLog* m_pBonusMoney;
};
