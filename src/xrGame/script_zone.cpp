////////////////////////////////////////////////////////////////////////////
//	Module 		: script_zone.cpp
//	Created 	: 10.10.2003
//  Modified 	: 10.10.2003
//	Author		: Dmitriy Iassenev
//	Description : Script zone object
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_zone.h"
#include "script_game_object.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "xrEngine/xr_collide_form.h"
#include "xrScriptEngine/script_callback_ex.h"
#include "game_object_space.h"

#ifdef DEBUG
#include "Level.h"
#include "debug_renderer.h"
#endif

CScriptZone::CScriptZone() {}
CScriptZone::~CScriptZone() {}
void CScriptZone::reinit() { inherited::reinit(); }
BOOL CScriptZone::net_Spawn(CSE_Abstract* DC)
{
    feel_touch.clear();

    if (!inherited::net_Spawn(DC))
        return (FALSE);

    return (TRUE);
}

void CScriptZone::net_Destroy() { inherited::net_Destroy(); }
void CScriptZone::shedule_Update(u32 dt)
{
    inherited::shedule_Update(dt);

    const Fsphere& s = GetCForm()->getSphere();
    Fvector P;
    XFORM().transform_tiny(P, s.P);
    feel_touch_update(P, s.R);
}

void CScriptZone::feel_touch_new(IGameObject* tpObject)
{
    CGameObject* l_tpGameObject = smart_cast<CGameObject*>(tpObject);
    if (!l_tpGameObject)
        return;

    callback(GameObject::eZoneEnter)(lua_game_object(), l_tpGameObject->lua_game_object());
}

void CScriptZone::feel_touch_delete(IGameObject* tpObject)
{
    CGameObject* l_tpGameObject = smart_cast<CGameObject*>(tpObject);

    if (!l_tpGameObject || l_tpGameObject->getDestroy())
        return;

    callback(GameObject::eZoneExit)(lua_game_object(), l_tpGameObject->lua_game_object());
}

void CScriptZone::net_Relcase(IGameObject* O)
{
    CGameObject* l_tpGameObject = smart_cast<CGameObject*>(O);
    if (!l_tpGameObject)
        return;

    xr_vector<IGameObject*>::iterator I = std::find(feel_touch.begin(), feel_touch.end(), O);
    if (I != feel_touch.end())
    {
        callback(GameObject::eZoneExit)(lua_game_object(), l_tpGameObject->lua_game_object());
    }
}

bool CScriptZone::feel_touch_contact(IGameObject* O) { return (((CCF_Shape*)GetCForm())->Contact(O)); }
#ifdef DEBUG
void CScriptZone::OnRender()
{
    if (!bDebug)
        return;
    GEnv.DRender->OnFrameEnd();
    // RCache.OnFrameEnd();
    Fvector l_half;
    l_half.set(.5f, .5f, .5f);
    Fmatrix l_ball, l_box;
    xr_vector<CCF_Shape::shape_def>& l_shapes = ((CCF_Shape*)GetCForm())->Shapes();
    xr_vector<CCF_Shape::shape_def>::iterator l_pShape;

    for (l_pShape = l_shapes.begin(); l_shapes.end() != l_pShape; ++l_pShape)
    {
        switch (l_pShape->type)
        {
        case 0:
        {
            Fsphere& l_sphere = l_pShape->data.sphere;
            l_ball.scale(l_sphere.R, l_sphere.R, l_sphere.R);
            Fvector l_p;
            XFORM().transform(l_p, l_sphere.P);
            l_ball.translate_add(l_p);
            Level().debug_renderer().draw_ellipse(l_ball, color_xrgb(0, 255, 255));
        }
        break;
        case 1:
        {
            l_box.mul(XFORM(), l_pShape->data.box);
            Level().debug_renderer().draw_obb(l_box, l_half, color_xrgb(0, 255, 255));
        }
        break;
        }
    }
}
#endif

bool CScriptZone::active_contact(u16 id) const
{
    xr_vector<IGameObject*>::const_iterator I = feel_touch.begin();
    xr_vector<IGameObject*>::const_iterator E = feel_touch.end();
    for (; I != E; ++I)
        if ((*I)->ID() == id)
            return (true);
    return (false);
}
