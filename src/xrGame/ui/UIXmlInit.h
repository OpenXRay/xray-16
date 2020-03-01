#pragma once
#include "xrUICore/XML/UIXmlInitBase.h"

class CUIDragDropList;
class CUISleepStatic;
class CUITabButtonMP;

class CUIXmlInit : public CUIXmlInitBase
{
public:
    CUIXmlInit();
    virtual ~CUIXmlInit();

    static bool InitDragDropListEx(CUIXml& xml_doc, LPCSTR path, int index, CUIDragDropListEx* pWnd);
    static bool InitTabButtonMP(CUIXml& xml_doc, LPCSTR path, int index, CUITabButtonMP* pWnd);
    static bool InitSleepStatic(CUIXml& xml_doc, LPCSTR path, int index, CUISleepStatic* pWnd);
    static bool InitHintWindow(CUIXml& xml_doc, LPCSTR path, int index, UIHintWindow* pWnd);
};
