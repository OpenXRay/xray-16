////////////////////////////////////////////////////////////////////////////
//	Module 		: aimers_weapon.h
//	Created 	: 04.04.2008
//  Modified 	: 08.04.2008
//	Author		: Dmitriy Iassenev
//	Description : weapon aimer class
////////////////////////////////////////////////////////////////////////////

#ifndef AIMERS_WEAPON_H_INCLUDED
#define AIMERS_WEAPON_H_INCLUDED

#include "aimers_base.h"

class CWeapon;

namespace aimers {

class weapon : public base
{
public:
							weapon	(
								CGameObject* object,
								LPCSTR animation_id,
								bool animation_start,
								Fvector const& target,
								LPCSTR bone0,
								LPCSTR bone1,
								LPCSTR weapon_bone0,
								LPCSTR weapon_bone1,
								CWeapon const& weapon
							);
	inline	Fmatrix const&	get_bone			(u32 const& bone_id) const;

private:
	typedef base			inherited;

private:
			void			compute_bone		(u32 const bone_id);

private:
	enum {
		bone_id0			= u32(0),
		bone_id1,
		weapon_bone_id0,
		weapon_bone_id1,
		parent_weapon_bone_id0,
	}; // enum

private:
	Fmatrix					m_result[2];
	Fmatrix					m_bones[5];
	Fmatrix					m_local_bones[5];
	CWeapon const&			m_weapon;
	u16						m_bones_ids[5];
}; // class weapon

} // namespace aimers

#include "aimers_weapon_inline.h"

#endif // #ifndef AIMERS_WEAPON_H_INCLUDED