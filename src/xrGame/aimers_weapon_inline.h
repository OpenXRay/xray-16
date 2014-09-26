////////////////////////////////////////////////////////////////////////////
//	Module 		: aimers_weapon_inline.h
//	Created 	: 04.04.2008
//  Modified 	: 08.04.2008
//	Author		: Dmitriy Iassenev
//	Description : weapon aimer class inline functions
////////////////////////////////////////////////////////////////////////////

#ifndef AIMERS_WEAPON_INLINE_H_INCLUDED
#define AIMERS_WEAPON_INLINE_H_INCLUDED

inline Fmatrix const& aimers::weapon::get_bone	(u32 const& bone_id) const
{
	VERIFY	(bone_id < 2);
	return	(m_result[bone_id]);
}

#endif // #ifndef AIMERS_WEAPON_INLINE_H_INCLUDED