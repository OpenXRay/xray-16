////////////////////////////////////////////////////////////////////////////
//	Module 		: UIHelper.h
//	Created 	: 17.01.2008
//	Author		: Evgeniy Sokolov
//	Description : UI Helper class
////////////////////////////////////////////////////////////////////////////

#ifndef UI_HELPER_H_INCLUDED
#define UI_HELPER_H_INCLUDED

class CUIXml;
class CUIWindow;
class CUIStatic;
class CUIScrollView;
class CUITextWnd;
class CUIProgressBar;
class CUIProgressShape;
class CUIFrameLineWnd;
class CUIFrameWindow;
class CUI3tButton;
class CUICheckButton;
class UIHint;
class CUIDragDropListEx;
class CUIDragDropReferenceList;
class CUIEditBox;

class UIHelper
{
public:
    UIHelper(){};
    ~UIHelper(){};

    static CUIWindow* CreateNormalWindow(CUIXml& xml, const char* ui_path, CUIWindow* parent, bool critical = true);
    static CUIStatic* CreateStatic(CUIXml& xml, const char* ui_path, CUIWindow* parent, bool critical = true);
    static CUIStatic* CreateStatic(CUIXml& xml, const char* ui_path, int index, CUIWindow* parent, bool critical = true);
    static CUIScrollView* CreateScrollView(CUIXml& xml, const char* ui_path, CUIWindow* parent, bool critical = true);
    static CUITextWnd* CreateTextWnd(CUIXml& xml, const char* ui_path, CUIWindow* parent, bool critical = true);
    static CUIProgressBar* CreateProgressBar(CUIXml& xml, const char* ui_path, CUIWindow* parent, bool critical = true);
    static CUIProgressShape* CreateProgressShape(CUIXml& xml, const char* ui_path, CUIWindow* parent, bool critical = true);
    static CUIFrameLineWnd* CreateFrameLine(CUIXml& xml, const char* ui_path, CUIWindow* parent, bool critical = true);
    static CUIFrameWindow* CreateFrameWindow(CUIXml& xml, const char* ui_path, CUIWindow* parent, bool critical = true);
    static CUI3tButton* Create3tButton(CUIXml& xml, const char* ui_path, CUIWindow* parent, bool critical = true);
    static CUI3tButton* Create3tButton(CUIXml& xml, const char* ui_path, int index, CUIWindow* parent, bool critical = true);
    static CUICheckButton* CreateCheck(CUIXml& xml, const char* ui_path, CUIWindow* parent, bool critical = true);
    static CUIEditBox* CreateEditBox(CUIXml& xml, const char* ui_path, CUIWindow* parent, bool critical = true);

    static UIHint* CreateHint(CUIXml& xml, const char* ui_path /*, CUIWindow* parent*/, bool critical = true);
    static CUIDragDropListEx* CreateDragDropListEx(CUIXml& xml, const char* ui_path, CUIWindow* parent, bool critical = true);
    static CUIDragDropListEx* CreateDragDropListEx(CUIXml& xml, const char* ui_path, int index, CUIWindow* parent, bool critical = true);
    static CUIDragDropReferenceList* CreateDragDropReferenceList(CUIXml& xml, const char* ui_path, CUIWindow* parent, bool critical = true);

}; // class UIHelper

#endif // UI_HELPER_H_INCLUDED
