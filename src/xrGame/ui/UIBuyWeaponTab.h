#pragma once

#include "xrUICore/TabControl/UITabControl.h"
#include "xrUICore/Static/UIStatic.h"

class CUIXml;

class CUIBuyWeaponTab : public CUITabControl
{
    typedef CUITabControl inherited;

public:
    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData);

    // public:
    // 				CUIBuyWeaponTab				();
    // 	virtual		~CUIBuyWeaponTab			();
    //
    // 	virtual void Init						(CUIXml* xml, char* path);
    // 	virtual void OnTabChange				(const shared_str& sCur, const shared_str& sPrev);
    // 			void SetActiveState				(bool bState = true);
    // private:
    // 	bool			m_bActiveState;
    // 	shared_str		m_sStubId;
};
