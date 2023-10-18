#pragma once
#include "xrUICore/Windows/UIWindow.h"
#include "xrServerEntities/alife_space.h"

class CUIXml;
class CUIStatic;
class CUITextWnd;
class UIArtefactParamItem;

class CUIArtefactParams final : public CUIWindow
{
public:
    CUIArtefactParams() : CUIWindow("Artefact Params") {}
    ~CUIArtefactParams() override;
    bool InitFromXml(CUIXml& xml);
    bool Check(const shared_str& af_section);
    void SetInfo(const shared_str& af_section);
    pcstr GetDebugType() override { return "CUIArtefactParams"; }

protected:
    UIArtefactParamItem* CreateItem(CUIXml& uiXml, pcstr section,
        const shared_str& translationId, const shared_str& translationId2 = nullptr);

    UIArtefactParamItem* CreateItem(CUIXml& uiXml, pcstr section,
        float magnitude, bool isSignInverse, const shared_str& unit,
        const shared_str& translationId, const shared_str& translationId2 = nullptr);

protected:
    UIArtefactParamItem* m_immunity_item[ALife::infl_max_count]{};
    UIArtefactParamItem* m_restore_item[ALife::eRestoreTypeMax]{};
    UIArtefactParamItem* m_additional_weight{};

    CUIStatic* m_Prop_line{};

}; // class CUIArtefactParams

// -----------------------------------

class UIArtefactParamItem final : public CUIStatic
{
public:
    UIArtefactParamItem();

    enum class InitResult
    {
        Failed,
        Normal,
        Plain
    };

    InitResult Init(CUIXml& xml, pcstr section);

    void SetDefaultValuesPlain(float magnitude, bool isSignInverse, const shared_str& unit);
    void SetCaption(LPCSTR name);
    void SetValue(float value);

    pcstr GetDebugType() override { return "UIArtefactParamItem"; }

protected:
    InitResult InitPlain(CUIXml& xml, pcstr section);

private:
    CUIStatic* m_caption{};
    CUITextWnd* m_value{};
    float m_magnitude;
    bool m_sign_inverse;
    shared_str m_unit_str;
    shared_str m_texture_minus;
    shared_str m_texture_plus;

}; // class UIArtefactParamItem
