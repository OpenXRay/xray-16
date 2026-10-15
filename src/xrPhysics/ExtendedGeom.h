#pragma once

#include "PHObject.h"
#include "xrPhysicsCore/IPhysicsCore.h"
#include "PhysicsCommon.h"
#include "MathUtils.h"
#include "Geometry.h" // Подключаем, чтобы видеть CPhysicsGeom
#include <cmath>

#ifdef DEBUG
#include "debug_output.h"
#endif

class IPhysicsShellHolder;

class CObjectContactCallback
{
    CObjectContactCallback* next;
    ObjectContactCallbackFun* callback;

public:
    CObjectContactCallback(ObjectContactCallbackFun* c) : callback(c)
    {
        next = NULL;
        VERIFY(c);
    }

    ~CObjectContactCallback() { xr_delete(next); }
    void Call(bool& collide, bool first, CPhysicsGeom* geometry1, CPhysicsGeom* geometry2,
        const Fvector& normal, const Fvector& position, SGameMtl* material1, SGameMtl* material2)
    {
        for (auto* current = this; current; current = current->next)
            current->callback(collide, first, geometry1, geometry2, normal, position, material1, material2);
    }

    void Add(ObjectContactCallbackFun* c)
    {
        VERIFY(c);
        VERIFY(callback != c);

        if (next)
            next->Add(c);
        else
            next = xr_new<CObjectContactCallback>(c);
    }

    bool HasCallback(ObjectContactCallbackFun* c)
    {
        for (CObjectContactCallback* i = this; i; i = i->next)
        {
            VERIFY(i->callback);
            if (c == i->callback)
                return true;
        }
        return false;
    }

    static void RemoveCallback(CObjectContactCallback*& callbacks, ObjectContactCallbackFun* c)
    {
        if (!callbacks)
            return;
        VERIFY(c);
        VERIFY(callbacks->callback);

        if (c == callbacks->callback)
        {
            CObjectContactCallback* del = callbacks;
            callbacks = callbacks->next;
            del->next = NULL;
            xr_delete(del);
        }
        else
        {
            for (CObjectContactCallback* i = callbacks; i->next; i = i->next)
            {
                if (i->next->callback == c)
                {
                    CObjectContactCallback* del = i->next;
                    i->next = i->next->next;
                    del->next = NULL;
                    xr_delete(del);
                    return;
                }
            }
        }
    }
};

// Заменяем dxGeomUserData* на CPhysicsGeom* везде ниже:

IC void dGeomUserDataSetObjectContactCallback(CPhysicsGeom* geom, ObjectContactCallbackFun* obj_callback)
{
    if (!geom) return;
    xr_delete(geom->object_callbacks);
    if (obj_callback)
        geom->object_callbacks = xr_new<CObjectContactCallback>(obj_callback);
}

IC void dGeomUserDataAddObjectContactCallback(CPhysicsGeom* geom, ObjectContactCallbackFun* obj_callback)
{
    if (!geom) return;
    if (geom->object_callbacks)
        geom->object_callbacks->Add(obj_callback);
    else
        dGeomUserDataSetObjectContactCallback(geom, obj_callback);
}

IC void dGeomUserDataRemoveObjectContactCallback(CPhysicsGeom* geom, ObjectContactCallbackFun* obj_callback)
{
    if (!geom) return;
    CObjectContactCallback::RemoveCallback(geom->object_callbacks, obj_callback);
}

IC void dGeomUserDataSetElementPosition(CPhysicsGeom* geom, u16 e_pos) { if(geom) geom->m_element_position = e_pos; }
IC void dGeomUserDataSetBoneId(CPhysicsGeom* geom, u16 bone_id) { if(geom) geom->m_bone_id = bone_id; }

IC void dGeomUserDataResetLastPos(CPhysicsGeom* geom)
{
    if (!geom) return;
    geom->last_pos.set(-INFINITY, -INFINITY, -INFINITY);
    geom->pushing_neg = false;
    geom->pushing_b_neg = false;
    geom->b_static_colide = true;

    geom->last_aabb_size.set(0, 0, 0);
    geom->last_aabb_pos.set(0, 0, 0);
}

XRPHYSICS_API bool IsCyliderContact(CPhysicsGeom* geom1, CPhysicsGeom* geom2);
