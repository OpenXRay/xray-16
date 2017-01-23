#pragma once
#include "xrCore/XML/XMLDocument.hpp"

class CUIXml : public XMLDocument
{
	int						m_dbg_id;
public:
			CUIXml			();
	virtual	~CUIXml			();

	virtual shared_str correct_file_name	(LPCSTR path, LPCSTR fn);
};