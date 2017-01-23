////////////////////////////////////////////////////////////////////////////
//	Module 		: vision_client_inline.h
//	Created 	: 11.06.2007
//  Modified 	: 11.06.2007
//	Author		: Dmitriy Iassenev
//	Description : vision client inline functions
////////////////////////////////////////////////////////////////////////////

#ifndef VISION_CLIENT_INLINE_H
#define VISION_CLIENT_INLINE_H

IC	CVisualMemoryManager &vision_client::visual	() const
{
	VERIFY	(m_visual);
	return	(*m_visual);
}

#endif // VISION_CLIENT_INLINE_H