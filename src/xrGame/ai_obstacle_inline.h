////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_obstacle_inline.h
//	Created 	: 02.04.2007
//  Modified 	: 06.04.2007
//	Author		: Dmitriy Iassenev
//	Description : ai obstacle class inline functions
////////////////////////////////////////////////////////////////////////////

#ifndef AI_OBSTACLE_INLINE_H
#define AI_OBSTACLE_INLINE_H

IC ai_obstacle::ai_obstacle(CGameObject* object) : m_crc(0)
{
    m_object = object;
    m_actual = false;

    m_min_box = MagicBox3(Fidentity, Fvector().set(.5, 1.f, .5f));
    m_min_box.Center().y += 1.f;
}

IC void ai_obstacle::compute()
{
    if (m_actual)
        return;

    m_actual = true;
    compute_impl();
}

IC const ai_obstacle::AREA& ai_obstacle::area()
{
    compute();
    return (m_area);
}

IC const ai_obstacle::AREA& ai_obstacle::danger_area()
{
    //	compute_danger			();
    return (m_danger_area);
}

IC const u32& ai_obstacle::crc()
{
    compute();
    return (m_crc);
}

IC const MagicBox3 ai_obstacle::min_box()
{
    //	compute					();
    return (m_min_box);
}

#endif // AI_OBSTACLE_INLINE_H
