#include "StdAfx.h"
#include "ActorCameraCollision.h"
#include "xrEngine/CameraBase.h"
#include "xrPhysicsCore/IPhysicsCore.h"

CPhysicsShell* actor_camera_shell = nullptr;
#ifdef DEBUG
BOOL dbg_draw_camera_collision = FALSE;
#endif
float camera_collision_character_skin_depth = .4f;
float camera_collision_character_shift_z = .3f;

namespace {
constexpr float skin = .04f;
void Viewport(const CCameraBase& camera, float near_plane, Fvector& size, Fmatrix& transform)
{
    size.z = near_plane * .5f;
    viewport_size(near_plane, camera, size.x, size.y);
    size.add(Fvector().set(skin, skin, skin));
    transform.identity();
    transform.i = camera.Right();
    transform.j = camera.Up();
    transform.k = camera.Direction();
    transform.c.mad(camera.Position(), camera.Direction(), near_plane * .5f);
}
}

bool test_camera_box(const Fvector& size, const Fmatrix& transform, IPhysicsShellHolder* actor)
{
    VERIFY(actor);
    auto* core = GetPhysicsCore();
    const auto shape = core->CreateBoxShape(size);
    Fquaternion rotation;
    rotation.set(transform);
    const bool blocked = !core->CheckShapePlacement(shape, transform.c, rotation, true, actor, true);
    core->DestroyCDBModel(shape);
    return blocked;
}
bool test_camera_collide(CCameraBase& camera, float near_plane, IPhysicsShellHolder* actor,
    Fvector& offset, float scale)
{
    Fvector size;
    Fmatrix transform;
    Viewport(camera, near_plane, size, transform);
    size.mul(scale);
    transform.c.mad(camera.Direction(), offset);
    return test_camera_box(size, transform, actor);
}
void collide_camera(CCameraBase& camera, float near_plane, IPhysicsShellHolder* actor)
{
    VERIFY(actor);
    Fvector size;
    Fmatrix transform;
    Viewport(camera, near_plane, size, transform);
    Fquaternion rotation;
    rotation.set(transform);
    auto* core = GetPhysicsCore();
    const auto shape = core->CreateBoxShape(size);
    Fvector position;
    if (core->FindFreeShapePlacement(shape, transform.c, rotation, position, .5f, 24, true, actor, true))
        camera.vPosition.mad(position, camera.Direction(), -near_plane * .5f);
    core->DestroyCDBModel(shape);
}
