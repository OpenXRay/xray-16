#pragma once

#include "CustomZone.h"
//#include "../Include/xrRender/KinematicsAnimated.h"
#include "../Include/xrRender/KinematicsAnimated.h"
#include "ZoneVisual.h"

#include "script_export_space.h"

class CHairsZone : public CVisualZone {
typedef				CVisualZone		inherited;		
public:
	virtual			void		Affect				(SZoneObjectInfo* O)		;
	virtual			void		Load				(LPCSTR section);

protected:
					float		m_min_speed_to_react;
	virtual			bool		BlowoutState		();
	virtual			void		CheckForAwaking		();

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CHairsZone)
#undef script_type_list
#define script_type_list save_type_list(CHairsZone)