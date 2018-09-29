#pragma once
#include "xrUICore/Windows/UIWindow.h"

#include "xrUICore/Windows/UIFrameWindow.h"
#include "xrUICore/Windows/UIFrameLineWnd.h"
#include "xrUICore/EditBox/UIEditBox.h"

class CUIXml;
class CUIScrollView;

class CUIKeyBinding : public CUIWindow
{
public:
    CUIKeyBinding();
    void InitFromXml(CUIXml& xml_doc, LPCSTR path);
#ifdef DEBUG
    void CheckStructure(CUIXml& xml_doc);
    bool IsActionExist(LPCSTR action, CUIXml& xml_doc);
#endif
protected:
    void FillUpList(CUIXml& xml_doc, LPCSTR path);

    CUIFrameLineWnd m_header[3];
    CUIFrameWindow m_frame;
    CUIScrollView* m_scroll_wnd;
};
