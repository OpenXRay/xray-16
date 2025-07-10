#include "StdAfx.h"

#include "UIWheelMenu.h"

constexpr pcstr WHEEL_MENU_XML = "wheel_menu.xml";

bool CUIWheelMenu::InitFromXml()
{
    CUIXml xml;
    xml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, WHEEL_MENU_XML, false);
    return false;
}
