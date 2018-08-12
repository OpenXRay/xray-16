#include "StdAfx.h"
#include "xrServer_svclient_validation.h"
#include "GameObject.h"
#include "Level.h"

bool is_object_valid_on_svclient(u16 id_entity)
{
    IGameObject* tmp_obj = Level().Objects.net_Find(id_entity);
    if (!tmp_obj)
        return false;

    CGameObject* tmp_gobj = smart_cast<CGameObject*>(tmp_obj);
    if (!tmp_gobj)
        return false;

    if (tmp_obj->getDestroy())
        return false;

    if (tmp_gobj->object_removed())
        return false;

    return true;
};
