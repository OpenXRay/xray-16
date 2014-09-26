#include "stdafx.h"
#include "../xrcdb/ispatial.h"
#include "icollidable.h"
#include "xr_collide_form.h"

ICollidable::ICollidable()		
{
	collidable.model					=	NULL;		
	ISpatial*		self				=	dynamic_cast<ISpatial*> (this);
	if (self)		self->spatial.type	|=	STYPE_COLLIDEABLE;
};
ICollidable::~ICollidable()		
{
	xr_delete		( collidable.model );	
};
