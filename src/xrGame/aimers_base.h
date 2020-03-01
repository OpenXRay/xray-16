////////////////////////////////////////////////////////////////////////////
//	Module 		: aimers_base.h
//	Created 	: 04.04.2008
//  Modified 	: 08.04.2008
//	Author		: Dmitriy Iassenev
//	Description : aimers base class
////////////////////////////////////////////////////////////////////////////

#ifndef AIMERS_BASE_H_INCLUDED
#define AIMERS_BASE_H_INCLUDED

#include "Include/xrRender/animation_motion.h"
#include "animation_movement_controller.h"
#include "Common/Noncopyable.hpp"

class CGameObject;
class IKinematics;
class IKinematicsAnimated;
class CBoneInstance;

namespace aimers
{
class base : private Noncopyable
{
public:
    base(CGameObject* object, LPCSTR animation_id, bool animation_start, Fvector const& target);

protected:
    template <u32 bone_count0, u32 bone_count1>
    inline void fill_bones(u32 const (&bones)[bone_count0], u16 const (&bones_ids)[bone_count1],
        Fmatrix (&local_bones)[bone_count1], Fmatrix (&global_bones)[bone_count1]);
    void aim_at_position(
        Fvector const& bone_position, Fvector const& object_position, Fvector object_direction, Fmatrix& result);
    void aim_at_direction(
        Fvector const& bone_position, Fvector const& object_position, Fvector const& object_direction, Fmatrix& result);
    static void callback(CBoneInstance* bone);

protected:
    Fmatrix m_start_transform;
    CGameObject& m_object;
    IKinematics& m_kinematics;
    IKinematicsAnimated& m_animated;
    Fvector const& m_target;
    MotionID m_animation_id;
    bool m_animation_start;
}; // class position_bone_aimer

} // namespace aimers

#include "aimers_base_inline.h"

#endif // #ifndef AIMERS_BASE_H_INCLUDED
