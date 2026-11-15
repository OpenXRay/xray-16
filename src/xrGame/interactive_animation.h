#pragma once

#include "physics_shell_animated.h"

class CGameObject;
class CBlend;
class CPhysicsGeom;
struct SGameMtl;

class interactive_animation : public physics_shell_animated
{
    typedef physics_shell_animated inherited;
    CBlend* blend;

public:
    interactive_animation(CPhysicsShellHolder* ca, CBlend* b);
    virtual ~interactive_animation();

public:
    virtual bool update(const Fmatrix& xrorm);

private:
    virtual void create_shell(CPhysicsShellHolder* O);
    bool collide();
    static void contact_callback(
        bool& do_colide, bool bo1,
        CPhysicsGeom* my_geom, CPhysicsGeom* oposite_geom,
        const Fvector& contact_normal, const Fvector& contact_pos,
        SGameMtl* material_1, SGameMtl* material_2);
};
