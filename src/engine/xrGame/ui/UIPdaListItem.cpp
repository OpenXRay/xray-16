#include "stdafx.h"

#include "UIPdaListItem.h"
#include "../actor.h"
#include "UIInventoryUtilities.h"
#include "../string_table.h"

#include "xrUIXmlParser.h"
#include "UIXmlInit.h"

#include "../../xrServerEntities/character_info.h"

#include "UIFrameWindow.h"
#include "../InventoryOwner.h"
#include "UICharacterInfo.h"
#include "UIStatic.h"

#define			PDA_CONTACT_CHAR		"pda_character.xml"

CUIPdaListItem::CUIPdaListItem()
{
	UIInfo = NULL;
}

CUIPdaListItem::~CUIPdaListItem()
{
}

void CUIPdaListItem::InitPdaListItem(Fvector2 pos, Fvector2 size)
{
	inherited::SetWndPos						(pos);
	inherited::SetWndSize						(size);

	CUIXml										uiXml;
	uiXml.Load									(CONFIG_PATH, UI_PATH, PDA_CONTACT_CHAR);

	CUIXmlInit xml_init;
	UIInfo = xr_new<CUICharacterInfo>			();
	UIInfo->SetAutoDelete						(true);
	AttachChild									(UIInfo);
	UIInfo->InitCharacterInfo					(Fvector2().set(0,0), size, PDA_CONTACT_CHAR);

	xml_init.InitAutoStaticGroup				(uiXml,"pda_char_auto_statics", 0, this);
}

void CUIPdaListItem::InitCharacter(CInventoryOwner* pInvOwner)
{
	VERIFY										(pInvOwner);
	UIInfo->InitCharacter						(pInvOwner->object_id());
}
