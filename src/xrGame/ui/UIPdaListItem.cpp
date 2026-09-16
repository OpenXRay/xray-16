#include "StdAfx.h"

#include "UIPdaListItem.h"
#include "../Actor.h"
#include "UIInventoryUtilities.h"
#include "xrEngine/StringTable/StringTable.h"

#include "xrUICore/XML/xrUIXmlParser.h"
#include "UIXmlInit.h"

#include "xrUICore/Windows/UIFrameWindow.h"
#include "../InventoryOwner.h"
#include "UICharacterInfo.h"
#include "xrUICore/Static/UIStatic.h"

#define PDA_CONTACT_CHAR "pda_character.xml"

CUIPdaListItem::CUIPdaListItem() : CUIWindow("CUIPdaListItem")
{
    UIMask = NULL;
    UIInfo = NULL;
}

CUIPdaListItem::~CUIPdaListItem() {}

void CUIPdaListItem::Init(float x, float y, float width, float height)
{
    inherited::SetWndPos(Fvector2().set(x, y));
    inherited::SetWndSize(Fvector2().set(width, height));

    CUIXml uiXml;
    bool xml_result = uiXml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, PDA_CONTACT_CHAR);
    R_ASSERT2(xml_result, "xml file not found");

    CUIXmlInit xml_init;
    UIInfo = xr_new<CUICharacterInfo>();
    UIInfo->SetAutoDelete(true);
    AttachChild(UIInfo);
    UIInfo->InitCharacterInfo(Fvector2().set(0, 0), Fvector2().set(width, height), PDA_CONTACT_CHAR);

    if (ShadowOfChernobylMode)
    {
        constexpr float iconX = 0.0f;
        constexpr float infoX = 108.0f;
        constexpr float valueOffset = 72.0f;

        if (CUIStatic* icon = UIInfo->GetIcon(CUICharacterInfo::eIcon))
        {
            icon->SetWndPos(iconX, icon->GetWndPos().y);
            icon->SetStretchTexture(true);
        }

        if (CUIStatic* iconOver = UIInfo->GetIcon(CUICharacterInfo::eIconOver))
            iconOver->SetWndPos(iconX, iconOver->GetWndPos().y);

        const CUICharacterInfo::UIItemType labels[] = {
            CUICharacterInfo::eName,
            CUICharacterInfo::eNameCaption,
            CUICharacterInfo::eRankCaption,
            CUICharacterInfo::eCommunityCaption,
            CUICharacterInfo::eReputationCaption,
            CUICharacterInfo::eRelationCaption,
        };
        for (const CUICharacterInfo::UIItemType type : labels)
        {
            if (CUIStatic* item = UIInfo->GetIcon(type))
            {
                item->SetWndPos(infoX, item->GetWndPos().y);
                item->SetWidth(width - infoX);
            }
        }

        const CUICharacterInfo::UIItemType values[] = {
            CUICharacterInfo::eRank,
            CUICharacterInfo::eCommunity,
            CUICharacterInfo::eReputation,
            CUICharacterInfo::eRelation,
        };
        for (const CUICharacterInfo::UIItemType type : values)
        {
            if (CUIStatic* item = UIInfo->GetIcon(type))
            {
                item->SetWndPos(infoX + valueOffset, item->GetWndPos().y);
                item->SetWidth(width - infoX - valueOffset);
            }
        }
    }

    xml_init.InitAutoStaticGroup(uiXml, "pda_char_auto_statics", 0, this);

    if (ShadowOfChernobylMode)
    {
        // The first automatic static is the online/activity indicator. Put it
        // in the gap between the portrait and the visible name text.
        if (CUIWindow* activityIndicator = FindChild("auto_static_0"))
            activityIndicator->SetWndPos(106.0f, 2.5f);
    }
}

void CUIPdaListItem::InitCharacter(CInventoryOwner* pInvOwner)
{
    VERIFY(pInvOwner);
    UIInfo->InitCharacter(pInvOwner->object_id());
}
