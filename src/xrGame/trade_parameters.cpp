////////////////////////////////////////////////////////////////////////////
//	Module 		: trade_parameters.cpp
//	Created 	: 13.01.2006
//  Modified 	: 13.01.2006
//	Author		: Dmitriy Iassenev
//	Description : trade parameters class
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "trade_parameters.h"

CTradeParameters* CTradeParameters::m_instance = 0;

void CTradeParameters::process(action_show, CInifile& ini_file, const shared_str& section)
{
    VERIFY(ini_file.section_exist(section));
    m_show.clear();
    CInifile::Sect& S = ini_file.r_section(section);
    auto I = S.Data.cbegin();
    auto E = S.Data.cend();
    for (; I != E; ++I)
        if (!(*I).second.size())
            m_show.disable((*I).first);
}
