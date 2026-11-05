#include "StdAfx.h"

#include "interactive_animation.h"

#include "xrPhysics/PhysicsShell.h"
#include "xrPhysics/ExtendedGeom.h"
#include "xrPhysics/MathUtils.h"

#include "Include/xrRender/KinematicsAnimated.h"

interactive_animation::interactive_animation(CPhysicsShellHolder* O, CBlend* b)
    : physics_shell_animated(O, false), blend(b)
{
}

interactive_animation::~interactive_animation() {}
bool interactive_animation::collide()
{
    physics_shell->CollideAll();

    return false;
}
bool interactive_animation::update(const Fmatrix& xrorm)
{
    if (!blend)
        return false;
    if (!inherited::update(xrorm))
        return false;

    if (blend->playing && collide())
    {
        if (blend->Callback)
            blend->Callback(blend);
        blend->blendPower *= 0.5f;
        blend->playing = false;
    }
    if (!blend->playing)
    {
        blend = 0;
        return false;
    }
    return true;
}

void interactive_animation::contact_callback(
    bool& do_colide, bool bo1,
    CPhysicsGeom* my_geom, CPhysicsGeom* oposite_geom,
    const Fvector& contact_normal, const Fvector& contact_pos,
    SGameMtl* material_1, SGameMtl* material_2)
{
    IPhysicsShellHolder* holder_1 = my_geom ? (IPhysicsShellHolder*)my_geom->get_callback_data() : nullptr;
    IPhysicsShellHolder* holder_2 = oposite_geom ? (IPhysicsShellHolder*)oposite_geom->get_callback_data() : nullptr;

    if (!holder_1) return;

    if (holder_2 && holder_1 == holder_2)
        return;
}

void interactive_animation::create_shell(CPhysicsShellHolder* O)
{
    inherited::create_shell(O);
    physics_shell->add_ObjectContactCallback(contact_callback);
}
