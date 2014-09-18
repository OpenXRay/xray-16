#pragma once

//#include "../../../PHCharacter.h"
#include "../../../../xrphysics/IPHCapture.h"
#include "../../../CaptureBoneCallback.h"
#include "../../../../include/xrrender/Kinematics.h"
#include <boost/noncopyable.hpp>

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateGroupDragAbstract CStateGroupDrag<_Object>

TEMPLATE_SPECIALIZATION
CStateGroupDragAbstract::CStateGroupDrag(_Object *obj) : inherited(obj)
{
}

TEMPLATE_SPECIALIZATION
CStateGroupDragAbstract::~CStateGroupDrag()
{
}

TEMPLATE_SPECIALIZATION
void CStateGroupDragAbstract::initialize()
{
	inherited::initialize();
	IKinematics*  K = object->EatedCorpse->Visual()->dcast_PKinematics( );
	VERIFY( K );
	CInifile *ini = K->LL_UserData();
	VERIFY( ini );

	if(!ini->section_exist("capture_used_bones") || !ini->line_exist("capture_used_bones","bones"))
	{
		m_failed = true;
		return;
	}
	LPCSTR bones = ini->r_string( "capture_used_bones","bones" );

	int				bone_number = _GetItemCount( bones );
	u16				*vbones = (u16*)_alloca(bone_number*sizeof(u16));
	u16				*I = vbones;
	u16				*E = vbones + bone_number;
	for ( ; I != E; ++I) {
		string32	sbone;
		_GetItem	( bones, int(I - vbones), sbone );
		*I			= K->LL_BoneID( sbone );
		VERIFY		( *I != BI_NONE );
	}
	struct callback : public CPHCaptureBoneCallback {
		IKinematics*  m_K;
		u16 const *use_bones;
		int			m_bone_number;
				callback	( IKinematics*  K, u16 const *ub, int const &bone_number ) :
			m_K				( K ),
			use_bones		( ub ),
			m_bone_number	( bone_number )
		{
		}

		bool	operator()	( u16 bone ) { 
			u16 bi = bone;
			for( ;m_K->LL_GetBoneRoot() != bi ;)
			{
				struct cmp_pred
				{
					cmp_pred(u16 i): m_id( i ){}
					u16 m_id;
					bool operator () (u16 id)
					{
						return id == m_id;
					}
				} cmp( bi );

				u16	const	*use_bones_end = use_bones + m_bone_number;
				if ( std::find_if( use_bones, use_bones_end, cmp) != use_bones_end )
					return	(true);

				bi			= m_K->LL_GetData( bi ).GetParentID();
			}
			return			(false); 
		}
	} cb( K, vbones, bone_number );

	object->character_physics_support()->movement()->PHCaptureObject(const_cast<CEntityAlive* >( object->EatedCorpse ), &cb);
	
	m_failed = false;
	
	IPHCapture *capture = object->character_physics_support()->movement()->PHCapture();
	if (capture && !capture->Failed()) {
		m_cover_vertex_id = object->Home->get_place_in_min_home();
		if (m_cover_vertex_id != u32(-1)) {
			m_cover_position	=  ai().level_graph().vertex_position(m_cover_vertex_id);
		} else m_cover_position	= object->Position();
		if (m_cover_vertex_id == u32(-1) || object->Position().distance_to(m_cover_position) < 2.f || !object->Home->at_min_home(m_cover_position)) {
			const CCoverPoint *point = object->CoverMan->find_cover(object->Home->get_home_point(), 1, object->Home->get_min_radius());
			if (point)
			{
				m_cover_vertex_id = point->level_vertex_id();
				if (m_cover_vertex_id != u32(-1))
				{
					m_cover_position	=  ai().level_graph().vertex_position(m_cover_vertex_id);
				}
			}
		}
	} else m_failed = true;
	m_corpse_start_position = object->EatedCorpse->Position();
	object->path().prepare_builder();
}

TEMPLATE_SPECIALIZATION
void CStateGroupDragAbstract::execute()
{
	if (m_failed) return;
	
	// Установить параметры движения
	object->set_action				(ACT_DRAG);
	object->anim().SetSpecParams	(ASP_MOVE_BKWD);

	if (m_cover_vertex_id != u32(-1)) {
		object->path().set_target_point			(m_cover_position, m_cover_vertex_id);
	} else {
		object->path().set_retreat_from_point	(object->EatedCorpse->Position());
	}

	object->path().set_generic_parameters	();
	object->anim().accel_activate			(eAT_Calm);

}

TEMPLATE_SPECIALIZATION
void CStateGroupDragAbstract::finalize()
{
	inherited::finalize();	

	// бросить труп
	if (object->character_physics_support()->movement()->PHCapture())
		object->character_physics_support()->movement()->PHReleaseObject();
}

TEMPLATE_SPECIALIZATION
void CStateGroupDragAbstract::critical_finalize()
{
	inherited::critical_finalize();

	// бросить труп
	if (object->character_physics_support()->movement()->PHCapture())
			object->character_physics_support()->movement()->PHReleaseObject();
}

TEMPLATE_SPECIALIZATION
bool CStateGroupDragAbstract::check_completion()
{
	if (m_failed) {
		return true;
	}

	if (!object->character_physics_support()->movement()->PHCapture())  {
		return true;
	}

	if (m_cover_vertex_id != u32(-1)) {		// valid vertex so wait path end
		if (object->Position().distance_to(m_cover_position) < 2.f) 
			return true;
	} else {								// invalid vertex so check distanced that passed
		if (m_corpse_start_position.distance_to(object->Position()) > object->Home->get_min_radius()) 
			return true;
	}

	return false;
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateGroupDragAbstract
