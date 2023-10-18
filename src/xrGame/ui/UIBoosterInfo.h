#pragma once
#include "xrUICore/Windows/UIWindow.h"
#include "EntityCondition.h"

class CUIXml;
class CUIStatic;
class CUITextWnd;
class UIBoosterInfoItem;

class CUIBoosterInfo final : public CUIWindow
{
public:
    CUIBoosterInfo();
    ~CUIBoosterInfo() override;

    bool InitFromXml(CUIXml& xml);
    void SetInfo(const shared_str& section);

    pcstr GetDebugType() override { return "CUIBoosterInfo"; }

protected:
    UIBoosterInfoItem* m_booster_items[eBoostExplImmunity]{};
    UIBoosterInfoItem* m_booster_satiety{};
    UIBoosterInfoItem* m_booster_anabiotic{};
    UIBoosterInfoItem* m_booster_time{};

    CUIStatic* m_Prop_line{};
}; // class CUIBoosterInfo

// -----------------------------------

class UIBoosterInfoItem final : public CUIWindow
{
public:
    UIBoosterInfoItem();

    void Init(CUIXml& xml, LPCSTR section);
    void SetCaption(LPCSTR name);
    void SetValue(float value);

    pcstr GetDebugType() override { return "UIBoosterInfoItem"; }

private:
    CUIStatic* m_caption{};
    CUITextWnd* m_value{};
    float m_magnitude;
    bool m_show_sign;
    shared_str m_unit_str;
    shared_str m_texture_minus;
    shared_str m_texture_plus;

}; // class UIBoosterInfoItem
