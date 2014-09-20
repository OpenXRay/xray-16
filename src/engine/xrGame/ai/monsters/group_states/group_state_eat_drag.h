#pragma once

#pragma warning(push)
#pragma warning(disable:4995)
#include <malloc.h>
#pragma warning(pop)

template<typename _Object>
class CStateGroupDrag : public CState<_Object> {
	typedef CState<_Object>		inherited;

	Fvector				m_cover_position;
	u32					m_cover_vertex_id;
	
	bool				m_failed;
	Fvector				m_corpse_start_position;

public:
						CStateGroupDrag		(_Object *obj);
	virtual				~CStateGroupDrag		();

	virtual void		initialize				();
	virtual	void		execute					();
	virtual void		finalize				();
	virtual void		critical_finalize		();
	virtual void		remove_links			(CObject* object) { inherited::remove_links(object);}

	virtual bool		check_completion		();
};


#include "group_state_eat_drag_inline.h"