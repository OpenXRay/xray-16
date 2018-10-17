#include "pch.hpp"
#include "xrUIXmlParser.h"

#ifdef XRUICORE_EXPORTS
#include "ui_base.h"
#endif

shared_str CUIXml::correct_file_name(pcstr path, pcstr fn)
{
#ifdef XRUICORE_EXPORTS
    if (0 == xr_strcmp(path, UI_PATH) || 0 == xr_strcmp(path, "UI"))
    {
        return UI().get_xml_name(fn);
    }
#endif

    return fn;
}

CUIXml::CUIXml()
{}

CUIXml::~CUIXml()
{}
