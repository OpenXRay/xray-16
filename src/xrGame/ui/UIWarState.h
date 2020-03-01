////////////////////////////////////////////////////////////////////////////
//	Module 		: UIWarState.h
//	Created 	: 15.04.2008
//	Author		: Evgeniy Sokolov
//	Description : UI war state (PDA) window class
////////////////////////////////////////////////////////////////////////////
#pragma once

#include "xrUICore/Hint/UIHint.h"

class CUIXml;
class CUIStatic;

class UIWarState : public UIHintWindow
{
    using inherited = UIHintWindow;

    CUIStatic* m_static;
    //	shared_str		m_def_texture;
    //	bool			m_installed;

public:
    UIWarState();
    virtual ~UIWarState() {};

    void InitXML(CUIXml& xml, LPCSTR att_name, CUIWindow* parent);
    void ClearInfo();
    bool UpdateInfo(LPCSTR icon, LPCSTR hint_text);
    void Draw() override;
}; // class UIWarState
