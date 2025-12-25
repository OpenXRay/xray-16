#pragma once

#include "xrUICore/Windows/UIWindow.h"

#include "xrServerEntities/alife_space.h"

class CInventoryItem;
class CUIXml;
class CUIStatic;
class UIArtefactParamItem;

class CUIArtefactParams final : public CUIWindow
{
public:
    CUIArtefactParams();
    ~CUIArtefactParams() override;

    bool InitFromXml(CUIXml& xml);
    bool Check(const shared_str& af_section) const;
    void SetInfo(const CInventoryItem& pInvItem);

    pcstr GetDebugType() override { return "CUIArtefactParams"; }

protected:
    UIArtefactParamItem* m_disp_condition{}; //Alundaio: Show AF Condition
    UIArtefactParamItem* m_immunity_item[ALife::eHitTypeMax - 3]{};
    UIArtefactParamItem* m_restore_item[ALife::eRestoreTypeMax]{};
    UIArtefactParamItem* m_additional_weight{};

    CUIStatic* m_Prop_line{};

}; // class CUIArtefactParams

// -----------------------------------

class UIArtefactParamItem final : public CUIStatic
{
public:
    UIArtefactParamItem(pcstr param_name);

    bool Init(CUIXml& xml, pcstr section, pcstr caption,
        u32 positive_color, u32 negative_color,
        float magnitude = 1.0f, bool is_sign_inverse = false, pcstr unit = "");

    void SetValue(float value);

    pcstr GetDebugType() override { return "UIArtefactParamItem"; }

private:
    CUIStatic m_value{ "value" };
    CUIStatic* m_caption{};
    float m_magnitude{ 1.0f };
    bool m_sign_inverse{ false };
    shared_str m_unit_str;
    shared_str m_texture_minus;
    shared_str m_texture_plus;
    u32 m_positive_color;
    u32 m_negative_color;
}; // class UIArtefactParamItem
