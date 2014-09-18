#include "stdafx.h"
#pragma hdrstop

#include "lephysics.h"
#include "scene.h"


#include "../../xrphysics/iphworld.h"
#include "../../xrcdb/xr_area.h"

//#include "../ECore/Editor/EditObject.h"

#include "sceneobject.h"
#include "ui_leveltools.h"
#include "mesh_data.h"
#include "spawnpoint.h"

CScenePhyscs	g_scene_physics;


bool	CScenePhyscs ::Simulating			()
{
	return !!physics_world();
}


BOOL  GetStaticCformData( const Fmatrix& parent,   CEditableMesh* mesh, CEditableObject* object, Fvector*  verts, int& vert_cnt, int& vert_it, CDB::TRI *faces , int& face_cnt,  int& face_it );
BOOL GetStaticCformData( CSceneObject* obj,mesh_build_data &data, bool b_selected_only )
{
	Fmatrix T 			= obj->_Transform();
    CEditableObject *O = obj->GetReference();


     for(EditMeshIt M=O->FirstMesh();M!=O->LastMesh();M++){
	  //	CSector* S = PortalUtils.FindSector(obj,*M);
	  //  int sect_num = S?S->m_sector_num:m_iDefaultSectorNum;
      //	if (!BuildMesh(T,O,*M,sect_num,l_verts,l_vert_cnt,l_vert_it,l_faces,l_face_cnt,l_face_it,l_smgroups,obj->_Transform()))

      if(!::GetStaticCformData(T,*M,O,data.l_verts,data.l_vert_cnt,data.l_vert_it,data.l_faces,data.l_face_cnt,data.l_face_it) )
        return FALSE;
      }



	return FALSE;
}
//BOOL GetMuStaticCformData( CSceneObject* obj,mesh_build_data &data, bool b_selected_only );
BOOL GetStaticCformData   ( ObjectList& lst, mesh_build_data &data, bool b_selected_only )
 {

 	BOOL bResult = TRUE;

    for(ObjectIt _F = lst.begin();_F!=lst.end();_F++)
	{
      //  pb->Inc((*_F)->Name);
      //  if (UI->NeedAbort()) break;
		if(b_selected_only && !(*_F)->Selected())
			continue;

        switch((*_F)->ClassID){
       // case OBJCLASS_LIGHT:
       //     bResult = BuildLight((CLight*)(*_F));
       //     break;
       // case OBJCLASS_GLOW:
       //     bResult = BuildGlow((CGlow*)(*_F));
        //    break;
      /// case OBJCLASS_PORTAL:
       //     l_portals.push_back(b_portal());
       //     BuildPortal(&l_portals.back(),(CPortal*)(*_F));
       //     break;
        case OBJCLASS_SCENEOBJECT:{
            CSceneObject *obj = (CSceneObject*)(*_F);
            if (obj->IsStatic()) 		
				bResult = GetStaticCformData(obj,data,b_selected_only);
            else if (obj->IsMUStatic())
				bResult = GetStaticCformData(obj,data,b_selected_only);
        }break;
/*        case OBJCLASS_GROUP:{
            CGroupObject* group = (CGroupObject*)(*_F);

            ObjectList 			grp_lst;
            group->GetObjects	(grp_lst);
            
            bResult = ParseStaticObjects(grp_lst, group->Name, b_selected_only);
        }break;   */
        }// end switch

    }
 //   UI->ProgressEnd(pb);
    return bResult;


 }




void GetBox( Fbox& box, const Fvector *verts, u32 cnt )
{
    box.invalidate();
	for( u32 i = 0; i < cnt; ++i )
    	box.modify( verts[i] );  
}

void	CScenePhyscs::OnSceneModified()
{
	ObjClassID cls = LTools->CurrentClassID();
    if( cls == OBJCLASS_SCENEOBJECT ||  cls == OBJCLASS_GROUP )
    	UpdateLevelCollision();

}
bool CScenePhyscs::CreateObjectSpace	(bool b_selected_only)
{

    ObjClassID cls = LTools->CurrentClassID();
    if(cls==OBJCLASS_DUMMY)	return FALSE;
	ESceneToolBase* pCurrentTool 	= Scene->GetOTool(cls);

	bool bResult	= true;

  	mesh_build_data build_data;
 
  	if(b_selected_only)
    {
   		if(pCurrentTool)
            pCurrentTool->GetStaticDesc( build_data.l_vert_cnt,build_data.l_face_cnt, b_selected_only,true );

   }else
    {
  	 	SceneToolsMapPairIt t_it 	= Scene->FirstTool();
     	SceneToolsMapPairIt t_end 	= Scene->LastTool();
        for (; t_it!=t_end; ++t_it)
        {
            ESceneToolBase* mt = t_it->second;
            if (mt)
            	mt->GetStaticDesc( build_data.l_vert_cnt,build_data.l_face_cnt, b_selected_only, true );
                      
               // if (!mt->ExportStatic(this,b_selected_only))
                   // {bResult = FALSE; break;}
        }
        
    }
	if(!bResult)
    	 return false;
     
	build_data.l_faces		= xr_alloc<CDB::TRI>	(build_data.l_face_cnt);
	build_data.l_verts		= xr_alloc<Fvector>(build_data.l_vert_cnt);
   if(b_selected_only)
   {
    	if(pCurrentTool)
           if (!pCurrentTool->GetStaticCformData(build_data,b_selected_only) )
               { bResult = false;}
    }else
    {
   	 	SceneToolsMapPairIt t_it 	= Scene->FirstTool();
     	SceneToolsMapPairIt t_end 	= Scene->LastTool();
        for (; t_it!=t_end; ++t_it)
        {
            ESceneToolBase* mt = t_it->second;
            if (mt)
                if (!mt->GetStaticCformData(build_data,b_selected_only))
                    { bResult = false; break;}
        }
    }
    VERIFY(!m_object_space)  ;
    hdrCFORM H;
    H.vertcount = build_data.l_vert_cnt;
    H.facecount = build_data.l_face_cnt;
    H.version	= CFORM_CURRENT_VERSION;
    GetBox( H.aabb,  build_data.l_verts, build_data.l_vert_cnt );
    VERIFY(!m_object_space);
    m_object_space = mesh_create_object_space(build_data.l_verts , build_data.l_faces , H, 0);

    xr_free( build_data.l_faces );
    xr_free( build_data.l_verts );

    b_update_level_collision = false;

    return    bResult;
}

CScenePhyscs::~CScenePhyscs			()
{
   //	DestroyAll			()  ;
    DestroyObjectSpace	()	;
    R_ASSERT( !m_object_space );
 }

void CScenePhyscs::DestroyObjectSpace	()
{
  destroy_object_space( m_object_space );
}
 void  CScenePhyscs::DestroyWorld			()
 {
	if(physics_world())
    	destroy_physics_world();

  }

void  CScenePhyscs::CreateWorld			()
{


    VERIFY(!physics_world());
    set_mtl_lib(&GMLib);


    CRenderDeviceBase *rd = &EDevice  ;
    VERIFY(m_object_space);
	create_physics_world( false, m_object_space,    0, rd );
}

void CreatePhysicsShellsSelected()
{
    ObjectList lst;
    if (Scene->GetQueryObjects(lst,OBJCLASS_SPAWNPOINT,1,1,0)){
    	for (ObjectIt it=lst.begin(); it!=lst.end(); it++){
	        CSpawnPoint* O = dynamic_cast<CSpawnPoint*>(*it); R_ASSERT(O);
            if(O->Selected()&&O->ObjectKinematics())
        				O->CreatePhysicsShell(&O->FTransform);
        }
    }
}
void   CScenePhyscs::	UseSimulatePoses	()
{
   ObjectList lst;
    if (Scene->GetQueryObjects(lst,OBJCLASS_SPAWNPOINT,1,1,0)){
    	for (ObjectIt it=lst.begin(); it!=lst.end(); it++){
	        CSpawnPoint* O = dynamic_cast<CSpawnPoint*>(*it); R_ASSERT(O);
            if(O->Selected())
            	O->UseSimulatePose();
       }
    }
}

void DestroyPhysicsShells()
{
    ObjectList lst;
    if (Scene->GetQueryObjects(lst,OBJCLASS_SPAWNPOINT,-1,-1,0)){
    	for (ObjectIt it=lst.begin(); it!=lst.end(); it++){
	        CSpawnPoint* O = dynamic_cast<CSpawnPoint*>(*it); R_ASSERT(O);
            O->DeletePhysicsShell();
        }
    }
}


 void  CScenePhyscs::	CreateShellsSelected()
 {
    if(b_update_level_collision)
    	DestroyObjectSpace();
 	if(!m_object_space)
    	CreateObjectSpace(false);
        
	CreateWorld					();
    CreatePhysicsShellsSelected	();
 }
 void  CScenePhyscs::	DestroyAll			()
 {
 	DestroyPhysicsShells();
    DestroyWorld();
 }