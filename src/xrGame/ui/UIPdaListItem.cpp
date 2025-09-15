#include "stdafx.h"

#include "UIPdaListItem.h"
#include "../actor.h"
#include "UIInventoryUtilities.h"
#include "UIXmlInit.h"

#include "xrUICore/Windows/UIFrameWindow.h"
#include "..\InventoryOwner.h"
#include "UICharacterInfo.h"
#include "xrUICore/Static/UIStatic.h"

#define PDA_CONTACT_CHAR "pda_character.xml"

CUIPdaListItem::CUIPdaListItem() : CUIWindow("CUIPdaListItem")
{
    UIMask = NULL;
    UIInfo = NULL;
}

CUIPdaListItem::~CUIPdaListItem()
{
}

void CUIPdaListItem::Init(u16 pizdabol)
{
    CUIXml uiXml;
    bool xml_result = uiXml.Load(CONFIG_PATH, UI_PATH, PDA_CONTACT_CHAR);
    R_ASSERT2(xml_result, "xml file not found");

    VERIFY(pInvOwner);
    UIInfo = xr_new<CUICharacterInfo>();
    UIInfo->SetAutoDelete(true);
    AttachChild(UIInfo);
    UIInfo->InitCharacter(pizdabol);

    CUIXmlInit xml_init;
    if (uiXml.NavigateToNode("mask_frame_window", 0))
    {
        UIMask = xr_new<CUIFrameWindow>("UIMask_pda");
        UIMask->SetAutoDelete(true);
        xml_init.InitFrameWindow(uiXml, "mask_frame_window", 0, UIMask);
        UIInfo->UIIcon().SetMask(UIMask);
    }

    xml_init.InitAutoStaticGroup(uiXml, "pda_char_auto_statics", 0, this);
}
