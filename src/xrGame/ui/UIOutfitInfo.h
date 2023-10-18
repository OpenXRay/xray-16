#pragma once
#include "xrUICore/Windows/UIWindow.h"
#include "xrUICore/ProgressBar/UIDoubleProgressBar.h"
#include "xrServerEntities/alife_space.h"

class CCustomOutfit;
class CHelmet;
class CUIStatic;
class CUIDoubleProgressBar;
class CUIXml;

class CUIOutfitImmunity final : public CUIWindow
{
public:
    CUIOutfitImmunity();

    bool InitFromXml(CUIXml& xml_doc, LPCSTR base_str, u32 hit_type);
    void SetProgressValue(float cur, float comp);

    pcstr GetDebugType() override { return "CUIOutfitImmunity"; }

protected:
    CUIStatic m_name; // texture + name
    CUIDoubleProgressBar m_progress;
    CUITextWnd m_value; // 100%
    float m_magnitude;

}; // class CUIOutfitImmunity

// -------------------------------------------------------------------------------------

class CUIOutfitInfo final : public CUIWindow
{
public:
    CUIOutfitInfo() : CUIWindow("CUIOutfitInfo") {}

    void InitFromXml(CUIXml& xml_doc);
    void UpdateInfo(CCustomOutfit* cur_outfit, CCustomOutfit* slot_outfit = nullptr);
    void UpdateInfo(CHelmet* cur_helmet, CHelmet* slot_helmet = nullptr);

    pcstr GetDebugType() override { return "CUIOutfitInfo"; }

protected:
    enum
    {
        max_count = ALife::eHitTypeMax - 3
    };

    CUIStatic* m_caption{};
    CUIStatic* m_Prop_line{};
    CUIOutfitImmunity* m_items[max_count]{};

}; // class CUIOutfitInfo
