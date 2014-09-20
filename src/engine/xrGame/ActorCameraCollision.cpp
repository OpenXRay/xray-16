#include "stdafx.h"

#include "Actor.h"

#include "../xrEngine/CameraBase.h"
#include "../xrEngine/gamemtllib.h"

#include "phworld.h"
#include "phcollidevalidator.h"
#include "PHShell.h"
#include "matrix_utils.h"
#include "ai/stalker/ai_stalker.h"
#include "GeometryBits.h"
#include "characterphysicssupport.h"
#ifdef DEBUG
#include "phdebug.h"
#endif
CPhysicsShell*	CActor::actor_camera_shell = NULL;

static bool cam_collided = false;
static bool cam_step	= false;
extern dJointGroupID ContactGroup;
static const float	camera_collision_sckin_depth = 0.04f;
static const float	camera_collision_character_sckin_depth = 0.5f;
static void	cammera_shell_collide_callback( bool& do_collide, bool bo1, dContact& c, SGameMtl* material_1, SGameMtl* material_2 )
{
	if( !do_collide )
		return;
	do_collide = false;
	SGameMtl* oposite_matrial	= bo1 ? material_1 : material_2 ;
	if(oposite_matrial->Flags.test(SGameMtl::flPassable))
		return;

	dxGeomUserData	*my_data			=	retrieveGeomUserData( bo1 ? c.geom.g1 : c.geom.g2 );
	dxGeomUserData	*oposite_data		=	retrieveGeomUserData( bo1 ? c.geom.g2 : c.geom.g1 ) ;

	VERIFY( my_data );
	if( oposite_data && oposite_data->ph_ref_object == my_data->ph_ref_object )
		return;
	if( c.geom.depth > camera_collision_sckin_depth/2.f )
	cam_collided = true;
	
	if( !cam_step )
		return;
	c.surface.mu = 0;
	c.surface.soft_cfm =0.01f;
	dJointID contact_joint	= dJointCreateContactSpecial(0, ContactGroup, &c);//dJointCreateContact(0, ContactGroup, &c);//
	CPHObject* obj = (CPHObject*)my_data->callback_data;
	VERIFY( obj );

	obj->Island().DActiveIsland()->ConnectJoint(contact_joint);

	if(bo1)
		dJointAttach			(contact_joint, dGeomGetBody(c.geom.g1), 0);
	else
		dJointAttach			(contact_joint, 0, dGeomGetBody(c.geom.g2));
	
}
static void cammera_shell_character_collide_callback( bool& do_collide, bool bo1, dContact& c, SGameMtl* material_1, SGameMtl* material_2 )
{
	dxGeomUserData	*oposite_data		=	retrieveGeomUserData( bo1 ? c.geom.g2 : c.geom.g1 ) ;
	do_collide =  false;
	if( !oposite_data || !oposite_data->ph_object || oposite_data->ph_object->CastType()!=CPHObject::tpCharacter)
		return;
	
	if( !smart_cast<CAI_Stalker*>(oposite_data->ph_ref_object) )
		return;

	do_collide =  true;
	cammera_shell_collide_callback(do_collide,bo1,c,material_1,material_2);
}


static void get_viewport_geom(Fvector &box, Fmatrix &form, const CCameraBase &camera , float _viewport_near )
{
	box.z = _viewport_near / 2.f;
	viewport_size ( _viewport_near, camera, box.x, box.y );
	form.identity();
	form.i.set( camera.Right() );
	form.j.set( camera.Up() );
	form.k.set( camera.Direction() );
	form.c.mad( camera.Position(), camera.Direction(), _viewport_near/2.f );
#ifdef DEBUG
	if( !_valid( form ) )
	{
		dump( "form", form );
		dump( "camera.Right()", camera.Right() );
		dump( "camera.Up()", camera.Up() );
		dump( "camera.Direction()", camera.Direction() );
		dump( "camera.Position()", camera.Position() );
		dump( "box", box );
		VERIFY(false);
	}
#endif
}

static const float actor_camera_hudge_mass = 10.f;
static const float actor_camera_hudge_mass_size = 10000000.f;
CPhysicsShell	* create_camera_shell(  )
{
	CGameObject	*actor = smart_cast<CGameObject	*>( Level().CurrentEntity() );
	VERIFY( Level().CurrentEntity() );
	CPhysicsShell	*shell = P_build_SimpleShell( actor, actor_camera_hudge_mass , true );
	CPhysicsElement* roote = shell->get_ElementByStoreOrder( 0 );
	Fobb obb; obb.m_halfsize.set(0.5f,0.5f,0.5f); obb.m_rotate.identity();obb.m_translate.set(0,0,0);
	//roote->add_Box(obb);
	CODEGeom* character_test_geom = smart_cast<CODEGeom*>(xr_new<CBoxGeom>(obb));
	character_test_geom->build( Fvector().set( 0, 0, 0 ) );//roote->mass_Center()
	character_test_geom->set_body( roote->get_body() );
	character_test_geom->set_ref_object(smart_cast<CPhysicsShellHolder*>(actor));
	CPHGeometryBits::set_ignore_static( *character_test_geom );
	roote->add_geom( character_test_geom );
	VERIFY( shell );
	shell->set_ApplyByGravity( false );
	shell->set_ObjectContactCallback( cammera_shell_collide_callback );
	character_test_geom->set_obj_contact_cb( cammera_shell_character_collide_callback );
	shell->set_ContactCallback( 0 );
	shell->set_CallbackData( smart_cast<CPHObject*>(shell) );
	dMass m;
	dMassSetSphere(&m,1,actor_camera_hudge_mass_size );
	dMassAdjust( &m, actor_camera_hudge_mass );
	shell->setEquelInertiaForEls( m );
	VERIFY( roote );
	roote->set_local_mass_center( Fvector().set(0,0,0) );
	VERIFY( roote->numberOfGeoms() );
	CODEGeom	*root_geom = roote->geometry( 0 );
	VERIFY( root_geom );
	root_geom->set_local_form_bt( Fidentity );
	shell->DisableCollision();
	shell->Disable();
	return shell;
}


#ifdef	DEBUG
BOOL dbg_draw_camera_collision = FALSE;
#endif
const u16 cam_correction_steps_num = 100;
void	update_current_entity_camera_collision( CPhysicsShellHolder* l_actor )
{
	
	if(	CActor::actor_camera_shell && 
		CActor::actor_camera_shell->get_ElementByStoreOrder( 0 )->PhysicsRefObject() 
			!= 
		 l_actor )
				destroy_physics_shell( CActor::actor_camera_shell );
	
	if( !CActor::actor_camera_shell )
			CActor::actor_camera_shell = create_camera_shell(  );
}

void get_camera_box( Fvector &box_size, Fmatrix &xform, const CCameraBase & camera, float _viewport_near)
{
	get_viewport_geom ( box_size, xform, camera, _viewport_near );
	box_size.add(Fvector().set(camera_collision_sckin_depth,camera_collision_sckin_depth,camera_collision_sckin_depth));
}
void get_old_camera_box( Fvector &old_box_size, Fmatrix &old_form, const CPhysicsElement *roote, const CBoxGeom* box )
{
	roote->GetGlobalTransformDynamic( &old_form );
	box->get_size( old_box_size );
}

void set_camera_collision( const Fvector &box_size, const Fmatrix &xform, CPhysicsElement *roote, CBoxGeom* box )
{
	box->set_size( box_size );
	CBoxGeom* character_collision_geom = smart_cast<CBoxGeom*>( roote->geometry( 1 ) );
	VERIFY( character_collision_geom );
	const Fvector character_collision_box_size = 
			Fvector().add(	box_size,
							Fvector().set(	camera_collision_character_sckin_depth,
											camera_collision_character_sckin_depth,
											camera_collision_character_sckin_depth
										)
						);
	character_collision_geom->set_size( character_collision_box_size );
	VERIFY( _valid(xform) );
	roote->SetTransform( xform );
}

void	do_collide_and_move(const Fmatrix &xform, CPhysicsShellHolder* l_actor, CPhysicsShell	*shell, CPhysicsElement *roote )
{
	///////////////////////////////////////////////////////////////////
	VERIFY( ph_world );
	VERIFY( !ph_world->Processing() );
	cam_collided = false;
	cam_step = false;
	VERIFY( l_actor );
	VERIFY( l_actor->character_physics_support() );
	VERIFY( l_actor->character_physics_support()->movement() );
	l_actor->character_physics_support()->movement()->CollisionEnable( FALSE );
	shell->EnableCollision();
	shell->CollideAll();

	if(cam_collided)
	{
		cam_step = true;
		for( u16 i = 0; i < cam_correction_steps_num; ++i )
		{	
			shell->set_LinearVel( Fvector().set(0,0,0) );
			shell->set_AngularVel( Fvector().set(0,0,0) );
			roote->setQuaternion( Fquaternion().set( xform ) );
			cam_collided = false;
			shell->PureStep();
			if( !cam_collided )
				break;
		}
		cam_step = false;
	}
	shell->DisableCollision();
	l_actor->character_physics_support()->movement()->CollisionEnable( TRUE );
	shell->Disable();
}

bool do_collide_not_move(const Fmatrix &xform, CPhysicsShellHolder* l_actor, CPhysicsShell	*shell, CPhysicsElement *roote)
{
		///////////////////////////////////////////////////////////////////
	VERIFY( ph_world );
	VERIFY( !ph_world->Processing() );
	cam_collided = false;
	cam_step = false;
	VERIFY( l_actor );
	VERIFY( l_actor->character_physics_support() );
	VERIFY( l_actor->character_physics_support()->movement() );
	l_actor->character_physics_support()->movement()->CollisionEnable( FALSE );
	shell->EnableCollision();
	shell->CollideAll();
	shell->DisableCollision();
	l_actor->character_physics_support()->movement()->CollisionEnable( TRUE );
	shell->Disable();
	return cam_collided;
}

bool test_camera_box( const Fvector &box_size, const Fmatrix &xform )
{
	CPhysicsShellHolder* l_actor = smart_cast<CPhysicsShellHolder*>( Level().CurrentEntity() );
	update_current_entity_camera_collision( l_actor );

	CPhysicsShell	*shell =  CActor::actor_camera_shell;
	VERIFY( shell );
	CPhysicsElement *roote = shell->get_ElementByStoreOrder( 0 );
	VERIFY( roote );
	CODEGeom	*root_geom = roote->geometry( 0 );
	VERIFY( root_geom );
	CBoxGeom* box  = smart_cast<CBoxGeom*>( root_geom );
	VERIFY( box );
	Fvector old_box_size; Fmatrix old_form; 
	get_old_camera_box( old_box_size, old_form, roote, box );

	set_camera_collision( box_size, xform, roote, box );
	bool ret = do_collide_not_move( xform, l_actor, shell, roote );
	set_camera_collision(  old_box_size, old_form, roote, box );

	return ret;
}

void	collide_camera( CCameraBase & camera, float _viewport_near  )
{
	CPhysicsShellHolder* l_actor = smart_cast<CPhysicsShellHolder*>( Level().CurrentEntity() );
	update_current_entity_camera_collision( l_actor );
	Fvector box_size; Fmatrix xform;
	get_camera_box( box_size, xform , camera, _viewport_near );
	CPhysicsShell	*shell =  CActor::actor_camera_shell;
	VERIFY( shell );
	CPhysicsElement *roote = shell->get_ElementByStoreOrder( 0 );
	VERIFY( roote );
	CODEGeom	*root_geom = roote->geometry( 0 );
	VERIFY( root_geom );
	CBoxGeom* box  = smart_cast<CBoxGeom*>( root_geom );
	VERIFY( box );
	Fvector old_box_size; Fmatrix old_form; 
	get_old_camera_box( old_box_size, old_form, roote, box );
	if( clamp_change( old_form, xform, EPS,EPS,EPS,EPS) &&  Fvector().sub( old_box_size, box_size ).magnitude() < EPS )
		return;
	set_camera_collision( box_size, xform, roote, box );
#ifdef	DEBUG
	if( dbg_draw_camera_collision )
		shell->dbg_draw_geometry( 1, D3DCOLOR_XRGB(0, 0, 255 ) );
#endif
	do_collide_and_move( xform, l_actor, shell, roote );
#ifdef	DEBUG
	if( dbg_draw_camera_collision )
		shell->dbg_draw_geometry( 1, D3DCOLOR_XRGB(0, 255, 0 ) );
#endif
	roote->GetGlobalPositionDynamic( &camera.vPosition );
	camera.vPosition.mad( camera.Direction(), -_viewport_near/2.f );
}