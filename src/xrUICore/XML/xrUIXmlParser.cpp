#include "pch.hpp"
#include "xrUIXmlParser.h"

#ifdef XRUICORE_EXPORTS
#include "ui_base.h"
#endif

shared_str CUIXml::correct_file_name(pcstr path, pcstr fn)
{
#ifdef XRUICORE_EXPORTS
    if (0 == xr_strcmp(path, UI_PATH))
    {
        return UI().get_xml_name(UI_PATH_WITH_DELIMITER, fn);
    }
    if (0 == xr_strcmp(path, UI_PATH_DEFAULT))
    {
        return UI().get_xml_name(UI_PATH_DEFAULT_WITH_DELIMITER, fn);
    }
#endif

    return fn;
}

CUIXml::CUIXml()
{}

CUIXml::~CUIXml()
{}
