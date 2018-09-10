#include "stdafx.h"
#include "xrUIXmlParser.h"

#ifdef XRGAME_EXPORTS
#include "xrGame/ui_base.h"
#endif // XRGAME_EXPORTS

shared_str CUIXml::correct_file_name(pcstr path, pcstr fn)
{
#ifdef XRGAME_EXPORTS
    if (0 == xr_strcmp(path, UI_PATH) || 0 == xr_strcmp(path, "UI"))
        return UI()->get_xml_name(fn);
#endif
    return fn;
}

CUIXml::CUIXml()
{
}

CUIXml::~CUIXml()
{}
