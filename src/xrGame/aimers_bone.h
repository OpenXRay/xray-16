////////////////////////////////////////////////////////////////////////////
//	Module 		: aimers_bone.h
//	Created 	: 04.04.2008
//  Modified 	: 08.04.2008
//	Author		: Dmitriy Iassenev
//	Description : bone aimer class
////////////////////////////////////////////////////////////////////////////

#ifndef AIMERS_BONE_H_INCLUDED
#define AIMERS_BONE_H_INCLUDED

#include "aimers_base.h"
#include "Include/xrRender/Kinematics.h"
#include "animation_movement_controller.h"
#include "game_object_space.h"

namespace aimers
{
template <u32 bone_count>
class bone : public base
{
public:
    bone(CGameObject* object, LPCSTR animation_id, bool animation_start, Fvector const& target,
        LPCSTR (&bones)[bone_count]);
    inline Fmatrix const& get_bone(u32 const& bone_id) const;

private:
    typedef base inherited;

private:
    void compute_bone(u32 const bone_id);
    void compute_bones(u32 const bone_id);

private:
    Fmatrix m_result[bone_count];
    Fmatrix m_bones[bone_count];
    u16 m_bones_ids[bone_count];
}; // class bone

} // namespace aimers

#include "aimers_bone_inline.h"

#endif // #ifndef AIMERS_BONE_H_INCLUDED
