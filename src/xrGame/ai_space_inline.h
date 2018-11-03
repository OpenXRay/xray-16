////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_space_inline.h
//	Created 	: 12.11.2003
//  Modified 	: 25.11.2003
//	Author		: Dmitriy Iassenev
//	Description : AI space class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC CEF_Storage& CAI_Space::ef_storage() const
{
    VERIFY(m_ef_storage);
    return (*m_ef_storage);
}

IC const CALifeSimulator& CAI_Space::alife() const
{
    VERIFY(m_alife_simulator);
    return (*m_alife_simulator);
}

IC const CALifeSimulator* CAI_Space::get_alife() const { return (m_alife_simulator); }
IC const CCoverManager& CAI_Space::cover_manager() const
{
    VERIFY(m_cover_manager);
    return (*m_cover_manager);
}

IC moving_objects& CAI_Space::get_moving_objects() const
{
    VERIFY(m_moving_objects);
    return (*m_moving_objects);
}

IC doors::manager& CAI_Space::doors() const
{
    VERIFY(m_doors_manager);
    return (*m_doors_manager);
}

IC CAI_Space& ai() { return CAI_Space::GetInstance(); }
