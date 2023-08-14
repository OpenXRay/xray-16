////////////////////////////////////////////////////////////////////////////
//	Module 		: UIMapLegend.h
//	Created 	: 03.06.2008
//	Author		: Evgeniy Sokolov
//	Description : UI Map Legend Wnd (PDA : Task) class
////////////////////////////////////////////////////////////////////////////

#ifndef UI_MAP_LEGEND_WND_H_INCLUDED
#define UI_MAP_LEGEND_WND_H_INCLUDED

#include "xrUICore/Windows/UIWindow.h"

class CUIXml;
class CUIFrameWindow;
class CUIScrollView;
class CUIStatic;
class CUI3tButton;
class CUICheckButton;
class CUIFrameLineWnd;
class UIHint;

class UIMapLegend final : public CUIWindow
{
private:
    typedef CUIWindow inherited;

public:
    UIMapLegend();
    ~UIMapLegend() override;

    void init_from_xml(CUIXml& xml, LPCSTR path);

    virtual void Show(bool status);
    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData);

    pcstr GetDebugType() override { return "UIMapLegend"; }

private: // m_
    CUIFrameWindow* m_background;
    CUIScrollView* m_list;

    CUIStatic* m_caption;
    CUI3tButton* m_btn_close;

}; // class UIMapLegend

// -------------------------------------------------------------------------------------------------

class UIMapLegendItem final : public CUIWindow
{
private:
    typedef CUIWindow inherited;

public:
    UIMapLegendItem();

    void init_from_xml(CUIXml& xml, int index);
    //	virtual void	Update				();

    pcstr GetDebugType() override { return "UIMapLegendItem"; }

private: // m_
    CUIStatic* m_image[4];
    CUIStatic* m_text;

}; // class UIMapLegendItem

#endif // UI_MAP_LEGEND_WND_H_INCLUDED
