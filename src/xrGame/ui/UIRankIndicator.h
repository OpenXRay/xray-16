#pragma once

#include "xrUICore/Windows/UIWindow.h"
class CUIStatic;
class CUIXml;
class CUIStatic;
class CUIRankIndicator : public CUIWindow
{
    enum
    {
        max_rank = 10,
    };
    CUIStatic* m_ranks[max_rank];
    u8 m_current;

public:
    CUIRankIndicator();
    virtual ~CUIRankIndicator();
    void InitFromXml(CUIXml& xml_doc);
    void SetRank(u8 team, u8 rank);
};
