#pragma once
#include "xrUICore/Windows/UIWindow.h"
#include "EntityCondition.h"

class CUIXml;
class CUIStatic;
class CUITextWnd;
class UIBoosterInfoItem;

class CUIBoosterInfo : public CUIWindow
{
public:
    CUIBoosterInfo();
    virtual ~CUIBoosterInfo();
    void InitFromXml(CUIXml& xml);
    void SetInfo(const shared_str& section);

protected:
    UIBoosterInfoItem* m_booster_items[eBoostExplImmunity];
    UIBoosterInfoItem* m_booster_satiety;
    UIBoosterInfoItem* m_booster_anabiotic;
    UIBoosterInfoItem* m_booster_time;

    CUIStatic* m_Prop_line;

}; // class CUIBoosterInfo

// -----------------------------------

class UIBoosterInfoItem : public CUIWindow
{
public:
    UIBoosterInfoItem();
    virtual ~UIBoosterInfoItem();

    void Init(CUIXml& xml, LPCSTR section);
    void SetCaption(LPCSTR name);
    void SetValue(float value);

private:
    CUIStatic* m_caption;
    CUITextWnd* m_value;
    float m_magnitude;
    bool m_show_sign;
    shared_str m_unit_str;
    shared_str m_texture_minus;
    shared_str m_texture_plus;

}; // class UIBoosterInfoItem
