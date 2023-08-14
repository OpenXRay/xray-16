#pragma once
#include "xrUICore/Windows/UIWindow.h"

#include "xrUICore/Windows/UIFrameWindow.h"
#include "xrUICore/Windows/UIFrameLineWnd.h"
#include "xrUICore/EditBox/UIEditBox.h"

class CUIXml;
class CUIScrollView;

class CUIKeyBinding final : public CUIWindow, public pureKeyMapChanged
{
public:
    CUIKeyBinding();
    ~CUIKeyBinding() override;
    void InitFromXml(CUIXml& xml_doc, LPCSTR path);
    pcstr GetDebugType() override { return "CUIKeyBinding"; }

protected:
    void FillUpList(CUIXml& xml_doc, LPCSTR path);

    void OnKeyMapChanged() override;

#ifdef DEBUG
    void CheckStructure(CUIXml& xml_doc);
    bool IsActionExist(LPCSTR action, CUIXml& xml_doc);
#endif

protected:
    bool m_isGamepadBinds{};

    std::array<CUIFrameLineWnd, 3> m_header;
    CUIFrameWindow m_frame;
    CUIScrollView* m_scroll_wnd{};
};
