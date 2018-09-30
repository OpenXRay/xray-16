#include "StdAfx.h"
#include "UIBuyWeaponTab.h"
#include "xrUICore/TabControl/UITabButton.h"

void CUIBuyWeaponTab::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    if (TAB_CHANGED == msg)
    {
        for (u32 i = 0; i < m_TabsArr.size(); ++i)
        {
            if (m_TabsArr[i] == pWnd)
            {
                m_sPushedId = m_TabsArr[i]->m_btn_id;

                OnTabChange(m_sPushedId, m_sPrevPushedId);
                m_sPrevPushedId = m_sPushedId;
                break;
            }
        }

        return;
    }

    return inherited::SendMessage(pWnd, msg, pData);
}
