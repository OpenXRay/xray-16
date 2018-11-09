#include "StdAfx.h"
#include "ExtendedGeom.h"
#include "dcylinder/dCylinder.h"
bool IsCyliderContact(const dContact& c)
{
    int geomClass = -1;
    if (dGeomGetBody(c.geom.g1))
    {
        geomClass = dGeomGetClass(retrieveGeom(c.geom.g1));
    }
    else
    {
        geomClass = dGeomGetClass(retrieveGeom(c.geom.g2));
    }

    // is_cyl= (geomClass==dCylinderClassUser);
    return (geomClass == dCylinderClassUser);
}

// dxGeomUserData* PHGeomGetUserData( dxGeom* geom )
//{
//	return dGeomGetUserData(geom);
//}

dxGeomUserData* PHRetrieveGeomUserData(dGeomID geom) { return retrieveGeomUserData(geom); }
void get_user_data(dxGeomUserData*& gd1, dxGeomUserData*& gd2, bool bo1, const dContactGeom& geom)
{
    if (bo1)
    {
        gd1 = retrieveGeomUserData(geom.g1);
        gd2 = retrieveGeomUserData(geom.g2);
    }
    else
    {
        gd2 = retrieveGeomUserData(geom.g1);
        gd1 = retrieveGeomUserData(geom.g2);
    }
}

//  bool dGeomUserDataHasCallback(dxGeom* geom,ObjectContactCallbackFun	*obj_callback)
//{
//	geom=retrieveGeom(geom);
//	if(geom&&dGeomGetUserData(geom)&&(dGeomGetUserData(geom))->object_callbacks)
//				return (dGeomGetUserData(geom))->object_callbacks->HasCallback(obj_callback);
//	else return false;
//}
