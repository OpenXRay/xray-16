////////////////////////////////////////////////////////////////////////////
//	Module 		: aimers_bone_inline.h
//	Created 	: 04.04.2008
//  Modified 	: 08.04.2008
//	Author		: Dmitriy Iassenev
//	Description : bone aimer class inline functions
////////////////////////////////////////////////////////////////////////////

#ifndef AIMERS_BONE_INLINE_H_INCLUDED
#define AIMERS_BONE_INLINE_H_INCLUDED

#define TEMPLATE_SPECIALIZATION		template <u32 bone_count>
#define BONE						aimers::bone<bone_count>

TEMPLATE_SPECIALIZATION
inline BONE::bone					(
		CGameObject* object,
		LPCSTR animation_id,
		bool animation_start,
		Fvector const& target,
		LPCSTR (&bones)[bone_count]
	) :
	inherited			( object, animation_id, animation_start, target )
{
	for (u32 i=0; i<bone_count; ++i)
		m_bones_ids[i]	= m_kinematics.LL_BoneID( bones[i] );

	compute_bones		(0);
}

TEMPLATE_SPECIALIZATION
inline Fmatrix const& BONE::get_bone(u32 const& bone_id) const
{
	VERIFY				( bone_id < bone_count );
	return				( m_result[bone_id] );
}

TEMPLATE_SPECIALIZATION
void BONE::compute_bones			(u32 const bone_id)
{
	compute_bone		( bone_id );

	VERIFY			( _valid(m_start_transform) );
	VERIFY			( _valid(m_result[bone_id]) );
	VERIFY			( _valid(Fmatrix(m_start_transform).invert()) );
	m_result[bone_id]	= Fmatrix(m_start_transform).invert().mulB_43(m_result[bone_id]).mulB_43(m_start_transform);
	VERIFY			( _valid(m_result[bone_id]) );

	if ( (bone_id + 1) == bone_count )
		return;

	Fmatrix&			bone_matrix = m_result[bone_id];
	Fvector				angles;
	bone_matrix.getXYZ	(angles);
	VERIFY			( _valid(angles) );
	VERIFY			( (bone_count - bone_id - 1) > 0 );
	angles.mul			(1.f/float(bone_count - bone_id - 1));
	bone_matrix.setXYZ	(angles);
	VERIFY			( _valid(bone_matrix) );

	CBoneInstance&		bone = m_kinematics.LL_GetBoneInstance( m_bones_ids[bone_id] );
	BoneCallback const&	old_callback = bone.callback();
	void*				old_callback_param = bone.callback_param();
	bone.set_callback	( bctCustom, &inherited::callback, &bone_matrix );

	compute_bones		( bone_id + 1 );

	bone.set_callback	( bctCustom, old_callback, old_callback_param );
}

TEMPLATE_SPECIALIZATION
void BONE::compute_bone				(u32 const bone_id)
{
	VERIFY				( bone_id < bone_count );

	u32 bones_ids[]		= { bone_id, bone_count - 1};
	Fmatrix				local_bones[bone_count];
	fill_bones			( bones_ids, m_bones_ids, local_bones, m_bones );

	if (bone_id < bone_count - 1) {
		aim_at_position		(
			m_bones[bone_id].c,
			m_bones[bone_count - 1].c,
			m_bones[bone_count - 1].k,
			m_result[bone_id]
		);
		return;
	}

#if 0
	Fmatrix const& bone					= m_bones[bone_id];
	Fvector const& current_direction	= bone.k;
	Fvector const target_direction		= Fvector().sub( m_target, bone.c ).normalize();
	Fvector	cross_product				= Fvector().crossproduct(current_direction, target_direction);
	float const sin_alpha				= cross_product.magnitude();
	float const cos_alpha				= current_direction.dotproduct(target_direction);
	m_result[bone_id].rotation			(cross_product.normalize(), atan2f(sin_alpha, cos_alpha));
#else // #if 0
	m_result[bone_id]					= Fidentity;
#endif // #if 0
}

#undef BONE
#undef TEMPLATE_SPECIALIZATION

#endif // #ifndef AIMERS_BONE_INLINE_H_INCLUDED