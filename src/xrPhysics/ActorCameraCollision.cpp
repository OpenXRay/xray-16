#include "StdAfx.h"

#include "ActorCameraCollision.h"

#include "xrEngine/CameraBase.h"
#include "xrEngine/GameMtlLib.h"

#include "PHShell.h"
#include "matrix_utils.h"
#include "IPhysicsShellHolder.h"
#include "xrEngine/xr_object.h" //--#SM+#--
#include "GeometryBits.h"
#include "PHWorld.h"
#include "PHCollideValidator.h"

#ifdef DEBUG
#include "debug_output.h"
#endif
CPhysicsShell* actor_camera_shell = NULL;
#ifdef DEBUG
BOOL dbg_draw_camera_collision = FALSE;
#endif
static bool cam_collided = false;
static bool cam_step = false;
extern dJointGroupID ContactGroup;
static const float camera_collision_sckin_depth = 0.04f;
float camera_collision_character_skin_depth = 0.4f;
float camera_collision_character_shift_z = 0.3f;
static const float camera_collision_character_gl_shift_y = 0.8f;
static void cammera_shell_collide_callback_common(
    bool& do_collide, bool bo1, dContact& c, SGameMtl* material_1, SGameMtl* material_2)
{
    if (!do_collide)
        return;
    do_collide = false;
    SGameMtl* oposite_matrial = bo1 ? material_1 : material_2;
    if (oposite_matrial->Flags.test(SGameMtl::flPassable))
        return;

    dxGeomUserData* my_data = retrieveGeomUserData(bo1 ? c.geom.g1 : c.geom.g2);
    dxGeomUserData* oposite_data = retrieveGeomUserData(bo1 ? c.geom.g2 : c.geom.g1);

    VERIFY(my_data);
    if (oposite_data && oposite_data->ph_ref_object == my_data->ph_ref_object)
        return;
    if (oposite_data && oposite_data->ph_ref_object &&
        !oposite_data->ph_ref_object->IsCollideWithActorCamera()) //--#SM+#--
        return;
    if (c.geom.depth > camera_collision_sckin_depth / 2.f)
        cam_collided = true;

    if (!cam_step)
        return;
    c.surface.mu = 0;

    dJointID contact_joint =
        dJointCreateContactSpecial(0, ContactGroup, &c); // dJointCreateContact(0, ContactGroup, &c);//
    CPHObject* obj = (CPHObject*)my_data->callback_data;
    VERIFY(obj);
#ifdef DEBUG
    if (dbg_draw_camera_collision)
        debug_output().DBG_DrawContact(c);
#endif
    obj->Island().DActiveIsland()->ConnectJoint(contact_joint);

    if (bo1)
        dJointAttach(contact_joint, dGeomGetBody(c.geom.g1), 0);
    else
        dJointAttach(contact_joint, 0, dGeomGetBody(c.geom.g2));
}
static const float soft_cfm_for_geometry = 0.01f;
static const float soft_cfm_for_controllers = 0.05f;

static void cammera_shell_collide_callback(
    bool& do_collide, bool bo1, dContact& c, SGameMtl* material_1, SGameMtl* material_2)
{
    c.surface.soft_cfm = soft_cfm_for_geometry;

    dxGeomUserData* oposite_data = retrieveGeomUserData(bo1 ? c.geom.g2 : c.geom.g1);

    if (oposite_data && oposite_data->ph_object && oposite_data->ph_object->CastType() == CPHObject::tpCharacter)
        c.surface.soft_cfm = soft_cfm_for_controllers;

    cammera_shell_collide_callback_common(do_collide, bo1, c, material_1, material_2);
}

static void cammera_shell_character_collide_callback(
    bool& do_collide, bool bo1, dContact& c, SGameMtl* material_1, SGameMtl* material_2)
{
    dxGeomUserData* oposite_data = retrieveGeomUserData(bo1 ? c.geom.g2 : c.geom.g1);
    do_collide = false;
    if (!oposite_data || !oposite_data->ph_object || oposite_data->ph_object->CastType() != CPHObject::tpCharacter)
        return;

    if (!oposite_data->ph_ref_object || !(oposite_data->ph_ref_object->IsStalker()))
        return;

    do_collide = true;
    c.surface.soft_cfm = soft_cfm_for_controllers;
    cammera_shell_collide_callback_common(do_collide, bo1, c, material_1, material_2);
}

static void get_viewport_geom(Fvector& box, Fmatrix& form, const CCameraBase& camera, float _viewport_near)
{
    box.z = _viewport_near / 2.f;
    tviewport_size(inl_ph_world().Device(), _viewport_near, camera, box.x, box.y);
    form.identity();
    form.i.set(camera.Right());
    form.j.set(camera.Up());
    form.k.set(camera.Direction());
    form.c.mad(camera.Position(), camera.Direction(), _viewport_near / 2.f);
#ifdef DEBUG
    if (!_valid(form))
    {
        dump("form", form);
        dump("camera.Right()", camera.Right());
        dump("camera.Up()", camera.Up());
        dump("camera.Direction()", camera.Direction());
        dump("camera.Position()", camera.Position());
        dump("box", box);
        VERIFY(false);
    }
#endif
}

static const float actor_camera_hudge_mass = 10.f;
static const float actor_camera_hudge_mass_size = 10000000.f;
CPhysicsShell* create_camera_shell(IPhysicsShellHolder* actor)
{
    VERIFY(actor);
    // CGameObject	*actor = smart_cast<CGameObject	*>( Level().CurrentEntity() );
    // VERIFY( Level().CurrentEntity() );
    CPhysicsShell* shell = P_build_SimpleShell(actor, actor_camera_hudge_mass, true);
    CPhysicsElement* roote = shell->get_ElementByStoreOrder(0);
    // Fobb obb; obb.m_halfsize.set(0.5f,0.5f,0.5f); obb.m_rotate.identity();obb.m_translate.set(0,0,0);
    Fcylinder cyl;
    cyl.m_center.set(0, -0.8f, 0);
    cyl.m_direction.set(0, 1, 0);
    cyl.m_height = 1.8f;
    cyl.m_radius = 0.5f;
    // roote->add_Box(obb);
    CODEGeom* character_test_geom = smart_cast<CODEGeom*>(new CCylinderGeom(cyl));
    character_test_geom->build(Fvector().set(0, 0, 0)); // roote->mass_Center()
    VERIFY(smart_cast<CPHElement*>(roote));
    CPHElement* eeroot = static_cast<CPHElement*>(roote);

    character_test_geom->set_body(eeroot->get_body());
    // character_test_geom->set_ref_object(smart_cast<CPhysicsShellHolder*>(actor));
    character_test_geom->set_ref_object(actor);
    CPHGeometryBits::set_ignore_static(*character_test_geom);
    roote->add_geom(character_test_geom);
    VERIFY(shell);
    shell->set_ApplyByGravity(false);
    shell->set_ObjectContactCallback(cammera_shell_collide_callback);
    character_test_geom->set_obj_contact_cb(cammera_shell_character_collide_callback);
    shell->set_ContactCallback(0);
    shell->set_CallbackData(smart_cast<CPHObject*>(shell));
    dMass m;
    dMassSetSphere(&m, 1, actor_camera_hudge_mass_size);
    dMassAdjust(&m, actor_camera_hudge_mass);
    shell->setEquelInertiaForEls(m);
    VERIFY(roote);
    roote->set_local_mass_center(Fvector().set(0, 0, 0));
    VERIFY(roote->numberOfGeoms());
    CODEGeom* root_geom = roote->geometry(0);
    VERIFY(root_geom);
    root_geom->set_local_form_bt(Fidentity);
    shell->DisableCollision();
    shell->Disable();
    return shell;
}

const u16 cam_correction_steps_num = 100;
void update_current_entity_camera_collision(IPhysicsShellHolder* l_actor)
{
    if (actor_camera_shell && actor_camera_shell->get_ElementByStoreOrder(0)->PhysicsRefObject() != l_actor)
        destroy_physics_shell(actor_camera_shell);

    if (!actor_camera_shell)
        actor_camera_shell = create_camera_shell(l_actor);
}

void get_camera_box(Fvector& box_size, Fmatrix& xform, const CCameraBase& camera, float _viewport_near)
{
    get_viewport_geom(box_size, xform, camera, _viewport_near);
    box_size.add(
        Fvector().set(camera_collision_sckin_depth, camera_collision_sckin_depth, camera_collision_sckin_depth));
}
void get_old_camera_box(Fvector& old_box_size, Fmatrix& old_form, const CPhysicsElement* roote, const CBoxGeom* box)
{
    roote->GetGlobalTransformDynamic(&old_form);
    box->get_size(old_box_size);
}

void set_camera_collision(const Fvector& box_size, const Fmatrix& xform, CPhysicsElement* roote, CBoxGeom* box)
{
    Fvector bs = box_size;
    bs.z = box_size.z * 2.f;
    bs.y = box_size.y * 1.5f;

    box->set_size(bs);
    Fmatrix m = Fidentity;
    m.c.z -= box_size.z;
    m.c.y -= box_size.y * 0.5f;
    box->set_local_form_bt(m);
    // CBoxGeom* character_collision_geom = smart_cast<CBoxGeom*>( roote->geometry( 1 ) );
    CCylinderGeom* character_collision_geom = smart_cast<CCylinderGeom*>(roote->geometry(1));
    VERIFY(character_collision_geom);
    const Fvector character_collision_box_size =
        Fvector().add(box_size, Fvector().set(camera_collision_character_skin_depth,
                                    camera_collision_character_skin_depth, camera_collision_character_skin_depth));
    // character_collision_geom->set_size( character_collision_box_size );
    character_collision_geom->set_radius(character_collision_box_size.x);
    VERIFY(_valid(xform));
    Fmatrix character_collision_geom_local_xform(Fmatrix().invert(xform));
    Fvector shift_fv = Fvector().mul(xform.k, camera_collision_character_shift_z);
    shift_fv.y = 0;
    character_collision_geom_local_xform.transform_dir(shift_fv);

    // character_collision_geom_local_xform.c.set( 0, -0.8f, 0 );
    character_collision_geom_local_xform.c.set(
        Fvector()
            .mul(character_collision_geom_local_xform.j, -camera_collision_character_gl_shift_y)
            .add(Fvector().set(shift_fv)));
    // character_collision_geom_local_xform.c.y =-0.8f;
    character_collision_geom->set_local_form_bt(character_collision_geom_local_xform);
    roote->SetTransform(xform, mh_clear);
}

void do_collide_and_move(
    const Fmatrix& xform, IPhysicsShellHolder* l_actor, CPhysicsShell* shell, CPhysicsElement* roote)
{
    ///////////////////////////////////////////////////////////////////
    VERIFY(ph_world);
    VERIFY(!ph_world->Processing());
    cam_collided = false;
    cam_step = false;
    VERIFY(l_actor);
    // VERIFY( l_actor->character_physics_support() );
    // VERIFY( l_actor->character_physics_support()->movement() );
    // l_actor->character_physics_support()->movement()->CollisionEnable( FALSE );
    l_actor->MovementCollisionEnable(false);
    shell->EnableCollision();
    shell->CollideAll();

    if (cam_collided)
    {
#ifdef DEBUG
// debug_output().PH_DBG_Clear();
// debug_output().DBG_OpenCashedDraw();
#endif
        cam_step = true;
        for (u16 i = 0; i < cam_correction_steps_num; ++i)
        {
            shell->set_LinearVel(Fvector().set(0, 0, 0));
            shell->set_AngularVel(Fvector().set(0, 0, 0));
            roote->setQuaternion(Fquaternion().set(xform));
            cam_collided = false;
            shell->PureStep();
            if (!cam_collided)
                break;
        }
        cam_step = false;
#ifdef DEBUG
// debug_output().DBG_ClosedCashedDraw( 5000 );
#endif
    }

    shell->DisableCollision();
    // l_actor->character_physics_support()->movement()->CollisionEnable( TRUE );
    l_actor->MovementCollisionEnable(true);
    shell->Disable();
}

bool do_collide_not_move(
    const Fmatrix& xform, IPhysicsShellHolder* l_actor, CPhysicsShell* shell, CPhysicsElement* roote)
{
    ///////////////////////////////////////////////////////////////////
    VERIFY(ph_world);
    VERIFY(!ph_world->Processing());
    cam_collided = false;
    cam_step = false;
    VERIFY(l_actor);
    // VERIFY( l_actor->character_physics_support() );
    // VERIFY( l_actor->character_physics_support()->movement() );
    // l_actor->character_physics_support()->movement()->CollisionEnable( FALSE );
    l_actor->MovementCollisionEnable(false);
    shell->EnableCollision();
    shell->CollideAll();
    shell->DisableCollision();
    // l_actor->character_physics_support()->movement()->CollisionEnable( TRUE );
    l_actor->MovementCollisionEnable(true);
    shell->Disable();
    return cam_collided;
}

bool test_camera_box(const Fvector& box_size, const Fmatrix& xform, IPhysicsShellHolder* l_actor)
{
    // CPhysicsShellHolder* l_actor = smart_cast<CPhysicsShellHolder*>( Level().CurrentEntity() );
    VERIFY(l_actor);
    update_current_entity_camera_collision(l_actor);

    CPhysicsShell* shell = actor_camera_shell;
    VERIFY(shell);
    CPhysicsElement* roote = shell->get_ElementByStoreOrder(0);
    VERIFY(roote);
    CODEGeom* root_geom = roote->geometry(0);
    VERIFY(root_geom);
    CBoxGeom* box = smart_cast<CBoxGeom*>(root_geom);
    VERIFY(box);
    Fvector old_box_size;
    Fmatrix old_form;
    get_old_camera_box(old_box_size, old_form, roote, box);

    set_camera_collision(box_size, xform, roote, box);
    bool ret = do_collide_not_move(xform, l_actor, shell, roote);
    set_camera_collision(old_box_size, old_form, roote, box);

    return ret;
}

// Test only, generate box from camera --#SM+#--
bool test_camera_collide(
    CCameraBase& camera, float _viewport_near, IPhysicsShellHolder* l_actor, Fvector& vPosOffset, float fBoxSizeMod)
{
    Fvector box_size;
    Fmatrix xform;
    get_camera_box(box_size, xform, camera, _viewport_near);
    box_size.mul(fBoxSizeMod);
    xform.c.mad(camera.Direction(), vPosOffset);

    return test_camera_box(box_size, xform, l_actor);
}

void collide_camera(CCameraBase& camera, float _viewport_near, IPhysicsShellHolder* l_actor)
{
    // CPhysicsShellHolder* l_actor = smart_cast<CPhysicsShellHolder*>( Level().CurrentEntity() );
    VERIFY(l_actor);
    update_current_entity_camera_collision(l_actor);
    Fvector box_size;
    Fmatrix xform;
    get_camera_box(box_size, xform, camera, _viewport_near);
    CPhysicsShell* shell = actor_camera_shell;
    VERIFY(shell);
    CPhysicsElement* roote = shell->get_ElementByStoreOrder(0);
    VERIFY(roote);
    CODEGeom* root_geom = roote->geometry(0);
    VERIFY(root_geom);
    CBoxGeom* box = smart_cast<CBoxGeom*>(root_geom);
    VERIFY(box);
    Fvector old_box_size;
    Fmatrix old_form;
    get_old_camera_box(old_box_size, old_form, roote, box);
    if (clamp_change(old_form, xform, EPS, EPS, EPS, EPS) && Fvector().sub(old_box_size, box_size).magnitude() < EPS)
        return;
    set_camera_collision(box_size, xform, roote, box);
#ifdef DEBUG
    if (dbg_draw_camera_collision)
    {
        debug_output().DBG_DrawMatrix(Fmatrix().translate(xform.c), 1);
        shell->dbg_draw_geometry(1, color_xrgb(0, 0, 255));
    }
#endif
    do_collide_and_move(xform, l_actor, shell, roote);
#ifdef DEBUG
    if (dbg_draw_camera_collision)
        shell->dbg_draw_geometry(1, color_xrgb(0, 255, 0));
#endif
    roote->GetGlobalPositionDynamic(&camera.vPosition);
    camera.vPosition.mad(camera.Direction(), -_viewport_near / 2.f);
}
