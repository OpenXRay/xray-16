#include "stdafx.h"
#pragma hdrstop

#include "ESceneObjectTools.h"
#include "../ECore/Editor/EditObject.h"
#include "../ECore/Editor/EditMesh.h"
#include "SceneObject.h"
#include "scene.h"
#include "../ECore/Editor/ExportSkeleton.h"
//.#include "clsid_game.h"
#include "../ECore/Editor/ui_main.h"
#include "ui_leveltools.h"
#include "../ECore/Editor/GeometryCollector.h"

#include "../../xrServerEntities/xrServer_Objects_Abstract.h"
#include "ESceneSpawnTools.h"
#include "GeometryPartExtractor.h"
#include "ResourceManager.h"

static bool s_draw_dbg = false;

//----------------------------------------------------
IC bool build_mesh(const Fmatrix& parent, CEditableMesh* mesh, CGeomPartExtractor* extractor, u32 game_mtl_mask, BOOL ignore_shader)
{
	bool bResult 			= true;
    mesh->GenerateVNormals	(&parent);
    // fill faces
    for (SurfFaces::const_iterator sp_it=mesh->GetSurfFaces().begin(); sp_it!=mesh->GetSurfFaces().end(); sp_it++){
		const IntVec& face_lst 	= sp_it->second;
        CSurface* surf 		= sp_it->first;
		int gm_id			= surf->_GameMtl(); 
        if (gm_id==GAMEMTL_NONE_ID){ 
        	ELog.DlgMsg		(mtError,"Object '%s', surface '%s' contain invalid game material.",mesh->Parent()->m_LibName.c_str(),surf->_Name());
        	bResult 		= FALSE; 
            break; 
        }
        SGameMtl* M 		= GMLib.GetMaterialByID(gm_id);
        if (0==M){
        	ELog.DlgMsg		(mtError,"Object '%s', surface '%s' contain undefined game material.",mesh->Parent()->m_LibName.c_str(),surf->_Name());
        	bResult 		= FALSE; 
            break; 
        }
        if (!M->Flags.is(game_mtl_mask)) continue;

        // check engine shader compatibility
        if (!ignore_shader){
            IBlender* 		B = EDevice.Resources->_FindBlender(surf->_ShaderName()); 
            if (FALSE==B){
                ELog.Msg	(mtError,"Can't find engine shader '%s'. Object '%s', surface '%s'. Export interrupted.",surf->_ShaderName(),mesh->Parent()->m_LibName.c_str(),surf->_Name());
                bResult 	= FALSE; 
                break; 
            }
            if (TRUE==B->canBeLMAPped()){ 
                ELog.Msg	(mtError,"Object '%s', surface '%s' contain static engine shader - '%s'. Export interrupted.",mesh->Parent()->m_LibName.c_str(),surf->_Name(),surf->_ShaderName());
                bResult 	= FALSE; 
                break; 
            }
        }
                                                  
        const st_Face* faces	= mesh->GetFaces();        	VERIFY(faces);
        const Fvector*	vn	 	= mesh->GetVNormals();		VERIFY(vn);
        const Fvector*	pts 	= mesh->GetVertices();		VERIFY(pts);
	    for (IntVec::const_iterator f_it=face_lst.begin(); f_it!=face_lst.end(); f_it++){
			const st_Face& face = faces[*f_it];
            Fvector 		v[3],n[3];
            parent.transform_tiny	(v[0],pts[face.pv[0].pindex]);
            parent.transform_tiny	(v[1],pts[face.pv[1].pindex]);
            parent.transform_tiny	(v[2],pts[face.pv[2].pindex]);
            parent.transform_dir	(n[0],vn[*f_it*3+0]); n[0].normalize();
            parent.transform_dir	(n[1],vn[*f_it*3+1]); n[1].normalize();
            parent.transform_dir	(n[2],vn[*f_it*3+2]); n[2].normalize();
            const Fvector2*	uv[3];
            mesh->GetFaceTC			(*f_it,uv);
            extractor->AppendFace	(surf,v,n,uv);
        }
        if (!bResult) break;
    }
    mesh->UnloadVNormals	();
    return bResult;
}

bool ESceneObjectTool::ExportBreakableObjects(SExportStreams* F)
{
	bool bResult = true;
    CGeomPartExtractor* extractor=0;

    Fbox 		bb;
    if (!GetBox(bb)) return false;

    extractor	= xr_new<CGeomPartExtractor>();
    extractor->Initialize(bb,EPS_L,2);

    UI->SetStatus	("Export breakable objects...");
	// collect verts&&faces
    {
	    SPBItem* pb = UI->ProgressStart(m_Objects.size(),"Prepare geometry...");
        for (ObjectIt it=m_Objects.begin(); it!=m_Objects.end(); it++){
	        pb->Inc();
            CSceneObject* obj 		= dynamic_cast<CSceneObject*>(*it); VERIFY(obj);
            if (obj->IsStatic()){
                CEditableObject *O 	= obj->GetReference();
                const Fmatrix& T 	= obj->_Transform();
                for(EditMeshIt M=O->FirstMesh();M!=O->LastMesh();M++)
                    if (!build_mesh	(T,*M,extractor,SGameMtl::flBreakable,FALSE)){bResult=false;break;}
            }
        }
	    UI->ProgressEnd(pb);
    }
    if (!extractor->Process())		bResult = false;
    // export parts
    if (bResult){
    	SBPartVec& parts			= extractor->GetParts();
	    SPBItem* pb = UI->ProgressStart(parts.size(),"Export Parts...");
        for (SBPartVecIt p_it=parts.begin(); p_it!=parts.end(); p_it++){
	        pb->Inc();
            SBPart*	P				= *p_it;
        	if (P->Valid()){
                // export visual
                AnsiString sn		= AnsiString().sprintf("meshes\\brkbl#%d.ogf",(p_it-parts.begin()));
                xr_string fn		= Scene->LevelPath()+sn.c_str();
                IWriter* W			= FS.w_open(fn.c_str()); R_ASSERT(W);
                if (!P->Export(*W,1)){
                    ELog.DlgMsg		(mtError,"Invalid breakable object.");
                    bResult 		= false;
                    break;
                }
                FS.w_close			(W);
                // export spawn object
                {
                    AnsiString entity_ref		= "breakable_object";
                    ISE_Abstract*	m_Data		= create_entity(entity_ref.c_str()); 	VERIFY(m_Data);
                    CSE_Visual* m_Visual		= m_Data->visual();	VERIFY(m_Visual);
                    // set params
                    m_Data->set_name			(entity_ref.c_str());
                    m_Data->set_name_replace	(sn.c_str());
                    m_Data->position().set		(P->m_RefOffset);
                    m_Data->angle().set			(P->m_RefRotate);
                    m_Visual->set_visual		(sn.c_str(),false);

					if (s_draw_dbg){
                        Fmatrix MX;
                        MX.setXYZi				(P->m_RefRotate);
                        MX.translate_over		(P->m_RefOffset);
                        Fvector DR				= {0,0,1};
                        MX.transform_dir		(DR);
                        Tools->m_DebugDraw.AppendLine(P->m_RefOffset,Fvector().mad(P->m_RefOffset,MX.k,1.f),0xFF0000FF,false,false);
                    }

                    NET_Packet					Packet;
                    m_Data->Spawn_Write			(Packet,TRUE);

                    F->spawn.stream.open_chunk	(F->spawn.chunk++);
                    F->spawn.stream.w			(Packet.B.data,Packet.B.count);
                    F->spawn.stream.close_chunk	();
                    destroy_entity				(m_Data);
                }
            }else{
            	ELog.Msg(mtError,"Can't export invalid part #%d",p_it-parts.begin());
            }
        }
	    UI->ProgressEnd(pb);
    }
    // clean up
    xr_delete(extractor);

    return bResult;
}
//----------------------------------------------------

IC BOOL OrientToNorm(Fvector& local_norm, Fmatrix33& form, Fvector& hs)
{
    Fvector * ax_pointer= (Fvector*)&form;
    int 	max_proj=0,min_size=0;
    for (u32 k=1; k<3; k++){
    	if (_abs(local_norm[k])>_abs(local_norm[max_proj]))
        	max_proj=k;
        if (hs[k]<hs[min_size])
        	min_size=k; 
    }
    if (min_size!=max_proj) return FALSE;
    if (local_norm[max_proj]<0.f){
    	local_norm.invert();
        ax_pointer[max_proj].invert();
        ax_pointer[(max_proj+1)%3].invert();
    }
    return TRUE;
}

bool ESceneObjectTool::ExportClimableObjects(SExportStreams* F)
{
	bool bResult                    = true;
    CGeomPartExtractor* extractor   = 0;

    Fbox 		bb;
    if (!GetBox(bb))
        return  false;

    extractor	                    = xr_new<CGeomPartExtractor>();
    extractor->Initialize           (bb,EPS_L,int_max);

    UI->SetStatus	("Export climable objects...");
	// collect verts&&faces
    {
	    SPBItem* pb                 = UI->ProgressStart(m_Objects.size(), "Prepare geometry...");
        for (ObjectIt it=m_Objects.begin(); it!=m_Objects.end(); it++)
        {
	        pb->Inc();
            CSceneObject* obj 		= dynamic_cast<CSceneObject*>(*it);
            VERIFY                  (obj);
            if (obj->IsStatic())
            {
                CEditableObject *O 	= obj->GetReference();
                const Fmatrix& T 	= obj->_Transform();
                
                for(EditMeshIt M =O->FirstMesh(); M!=O->LastMesh(); M++)
                    if (!build_mesh	(T, *M, extractor, SGameMtl::flClimable, TRUE))
                    {
                      bResult       = false;
                      break;
                    }
            }
        }
	    UI->ProgressEnd(pb);
    }
    if (!extractor->Process())
        bResult                     = false;

    // export parts
    if (bResult)
    {
    	SBPartVec& parts			= extractor->GetParts();
	    SPBItem* pb                 = UI->ProgressStart(parts.size(),"Export Parts...");
        for (SBPartVecIt p_it=parts.begin(); p_it!=parts.end(); p_it++)
        {
	        pb->Inc                 ();
            SBPart*	P				= *p_it;
        	if (P->Valid())
            {
                // export visual
                AnsiString sn		            = AnsiString().sprintf("clmbl#%d",(p_it-parts.begin()));

				Fvector local_normal	        = {0,0,0};

                LPCSTR mat_name = NULL;
                for (SBFaceVecIt it=P->m_Faces.begin(); it!=P->m_Faces.end(); it++)
                {
                	for (u32 k=0; k<3; k++)
                        local_normal.add	        ((*it)->n[k]);

                    mat_name     = (*it)->surf->_GameMtlName();
                }

                local_normal.normalize_safe		();
                
                // export spawn object
                {
                    AnsiString entity_ref		= "climable_object";
                    ISE_Abstract*	m_Data		= create_entity(entity_ref.c_str()); 	VERIFY(m_Data);
                    ISE_Shape* m_Shape			= m_Data->shape();                      VERIFY(m_Shape);
//					CSE_Visual* m_Visual		= m_Data->visual();	VERIFY(m_Visual);
                    // set params
                    m_Data->set_name			(entity_ref.c_str());
                    m_Data->set_name_replace	(sn.c_str());
                    // set shape
                    CShapeData::shape_def		shape;
                    shape.type					= CShapeData::cfBox;
                    shape.data.box.scale		((P->m_BBox.max.x-P->m_BBox.min.x)*0.5f,
                    							(P->m_BBox.max.y-P->m_BBox.min.y)*0.5f,
                                                (P->m_BBox.max.z-P->m_BBox.min.z)*0.5f);
                    m_Shape->assign_shapes		(&shape,1);
					// orientate object
	          		if (!OrientToNorm(local_normal,P->m_OBB.m_rotate,P->m_OBB.m_halfsize))
                    {
                    	ELog.Msg(mtError,"Invalid climable object found. [%3.2f, %3.2f, %3.2f]",VPUSH(P->m_RefOffset));
					}
                    else
                    {
                        Fmatrix M; M.set			(P->m_OBB.m_rotate.i,P->m_OBB.m_rotate.j,P->m_OBB.m_rotate.k,P->m_OBB.m_translate);
                        M.getXYZ					(P->m_RefRotate); // не i потому что в движке так
                        m_Data->position().set		(P->m_RefOffset); 
                        m_Data->angle().set			(P->m_RefRotate);

                        m_Data->set_additional_info((void*)mat_name);
                        NET_Packet					Packet;
                        m_Data->Spawn_Write			(Packet,TRUE);

                        F->spawn.stream.open_chunk	(F->spawn.chunk++);
                        F->spawn.stream.w			(Packet.B.data,Packet.B.count);
                        F->spawn.stream.close_chunk	();

						if (s_draw_dbg)
                        {
                            Tools->m_DebugDraw.AppendOBB(P->m_OBB);
                            M.transform_dir				(local_normal);
                            Tools->m_DebugDraw.AppendLine(P->m_RefOffset,Fvector().mad(P->m_RefOffset,local_normal,1.f));
                        }
                    }
                    destroy_entity				(m_Data);
                }
            }else
            {
            	ELog.Msg(mtError,"Can't export invalid part #%d",p_it-parts.begin());
            }
        }
	    UI->ProgressEnd     (pb);
    }
    // clean up
    xr_delete               (extractor);

    return                  bResult;
}

