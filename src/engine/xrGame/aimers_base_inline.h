////////////////////////////////////////////////////////////////////////////
//	Module 		: aimers_base_inline.h
//	Created 	: 04.04.2008
//  Modified 	: 08.04.2008
//	Author		: Dmitriy Iassenev
//	Description : aimers base class inline functions
////////////////////////////////////////////////////////////////////////////

#ifndef AIMERS_BASE_INLINE_H_INCLUDED
#define AIMERS_BASE_INLINE_H_INCLUDED

template <u32 bone_count0, u32 bone_count1>
inline void aimers::base::fill_bones	(
		u32 const	(&bones)		[bone_count0],
		u16 const	(&bones_ids)	[bone_count1],
		Fmatrix		(&local_bones)	[bone_count1],
		Fmatrix		(&global_bones)	[bone_count1]
	)
{
	STATIC_CHECK						(bone_count0 <= bone_count1, invalid_arrays_passed);

	u16 const root_bone_id				= m_kinematics.LL_GetBoneRoot();
	CBoneInstance& root_bone			= m_kinematics.LL_GetBoneInstance(root_bone_id);
	BoneCallback callback				= root_bone.callback();
	void* callback_params				= root_bone.callback_param();

	if (!m_animation_start || !m_object.animation_movement() || !m_object.animation_movement()->IsBlending())
		root_bone.set_callback			( bctCustom, 0, 0 );

	u32 const channel_id				= 1;
	u32 const channel_mask				= 1 << channel_id;

	if (m_animation_start) {
		for (u16 i=0; i<MAX_PARTS; ++i) {
			u32 const blend_count		= m_animated.LL_PartBlendsCount(i);
			for (u32 j=0; j<blend_count; ++j) {
				CBlend* const blend		= m_animated.LL_PartBlend(i, j);
				CBlend* const new_blend	= m_animated.LL_PlayCycle( i, blend->motionID, TRUE, 0, 0, channel_id );
				VERIFY				(new_blend);
				*new_blend				= *blend;
				new_blend->channel		= channel_id;
			}
		}
	}
	else {
		for (u16 i=0; i<MAX_PARTS; ++i) {
			CBlend* const blend			= m_animated.LL_PlayCycle( i, m_animation_id, 0, 0, 0, channel_id );
			if (blend)
				blend->timeCurrent		= m_animation_start ? 0.f : ( blend->timeTotal - (SAMPLE_SPF + EPS) );
		}
	}

	{
		u32 const*						i = bones;
		u32 const*						e = bones + bone_count0;
		for ( ; i != e; ++i) {
			VERIFY						( *i < bone_count1 );
			m_kinematics.Bone_GetAnimPos( local_bones[*i], bones_ids[*i], channel_mask, false );

			VERIFY						( _valid(local_bones[*i]) );
			VERIFY						( _valid(m_start_transform) );
			global_bones[*i].mul_43		( m_start_transform, local_bones[*i] );
			VERIFY						( _valid(global_bones[*i]) );
		}
	}

	for (u16 i=0; i<MAX_PARTS; ++i)
		m_animated.LL_CloseCycle		( i, channel_mask );

	root_bone.set_callback				( bctCustom, callback, callback_params );
}

#endif // #ifndef AIMERS_BASE_INLINE_H_INCLUDED