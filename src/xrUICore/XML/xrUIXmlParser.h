#pragma once
#include "xrCore/XML/XMLDocument.hpp"

class XRUICORE_API CUIXml : public XMLDocument
{
    int m_dbg_id;

public:
    CUIXml();
    virtual ~CUIXml();

    virtual shared_str correct_file_name(pcstr path, pcstr fn);
};
