#include "StdAfx.h"
#include "UIPdaMsgListItem.h"
#include "xrUICore/XML/xrUIXmlParser.h"
#include "UIXmlInit.h"

void CUIPdaMsgListItem::SetFont(CGameFont* pFont)
{
    UITimeText.SetFont(pFont);
    UICaptionText.SetFont(pFont);
    UIMsgText.SetFont(pFont);
}

void CUIPdaMsgListItem::InitPdaMsgListItem(const Fvector2& size)
{
    inherited::SetWndSize(size);

    CUIXml uiXml;
    uiXml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "maingame_pda_msg.xml");
    AttachChild(&UIIcon);
    CUIXmlInit::InitStatic(uiXml, "icon_static", 0, &UIIcon);

    AttachChild(&UITimeText);
    CUIXmlInit::InitTextWnd(uiXml, "time_static", 0, &UITimeText);

    AttachChild(&UICaptionText);
    CUIXmlInit::InitTextWnd(uiXml, "caption_static", 0, &UICaptionText);

    AttachChild(&UIMsgText);
    CUIXmlInit::InitTextWnd(uiXml, "msg_static", 0, &UIMsgText);
}
