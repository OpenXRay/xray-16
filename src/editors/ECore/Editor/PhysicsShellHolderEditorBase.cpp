#include "stdafx.h"
#pragma hdrstop

#include "PhysicsShellHolderEditorBase.h"
#include "../../xrphysics/physicsshell.h"
#	include "GameMtlLib.h"
//CObjectList	 Objects;
static void SetBoneMaterials( IKinematics &K )
{
	const u16 count =  K.LL_BoneCount();
    for(u16 i = 0; i < count; ++i)
    {
		CBoneData& bd =  K.LL_GetData( i );
        if (*(bd.game_mtl_name))
	        bd.game_mtl_idx = GMLib.GetMaterialIdx( bd.game_mtl_name.c_str() );
        else
        	bd.game_mtl_idx = 0 ;
    }
}


void  CPhysicsShellHolderEditorBase::CreatePhysicsShell( Fmatrix*	obj_xform )
{
    IKinematics* K = ObjectKinematics();
    if(!K)
    	return;
    VERIFY( K );
    string1024	s;
	if   ( !can_create_phys_shell( s, *this ) )
    {
    	Msg( s );
        return;
    }
    m_object_xform.set(*obj_xform);
    if(K->dcast_RenderVisual())
   	 		SetBoneMaterials( *K );
     K->CalculateBones_Invalidate();
    K->CalculateBones(TRUE);
    m_physics_shell = P_build_Shell( this, false );
    K->CalculateBones_Invalidate();
    K->CalculateBones(TRUE);

}

void  CPhysicsShellHolderEditorBase::DeletePhysicsShell	()
{
		destroy_physics_shell(m_physics_shell);
   		m_object_xform = Fidentity;
}

void  CPhysicsShellHolderEditorBase::UpdateObjectXform(Fmatrix &obj_xform)
{
	if(m_physics_shell)
    {
    	IKinematics*	K = ObjectKinematics();
        VERIFY(K);
        // K->CalculateBones();
        m_physics_shell->InterpolateGlobalTransform( &m_object_xform );
    }
    obj_xform.set(m_object_xform);
}
 void			CPhysicsShellHolderEditorBase::ApplyDragForce		( const Fvector &force )
 {
 	VERIFY( m_physics_shell );
    m_physics_shell->applyGravityAccel( force );
 }
