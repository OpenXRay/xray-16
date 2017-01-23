#include "stdafx.h"
#include "xrCDB/ISpatial.h"
#include "ICollidable.h"
#include "xr_collide_form.h"
// XXX: rename this file to CollidableBase.cpp
CollidableBase::CollidableBase()
{
    CForm = nullptr;
    ISpatial* self = dynamic_cast<ISpatial*> (this);
    if (self) self->GetSpatialData().type |= STYPE_COLLIDEABLE;
};
CollidableBase::~CollidableBase()
{
    xr_delete(CForm);
};
