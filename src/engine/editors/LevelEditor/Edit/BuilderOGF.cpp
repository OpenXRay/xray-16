//----------------------------------------------------
// file: BuilderRModel.cpp
//----------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "Builder.h"

#include "Scene.h"
#include "../ECore/Editor/EditObject.h"
#include "../ECore/Editor/EditMesh.h"
#include "SceneObject.h"
#include "ESceneAIMapTools.h"
//----------------------------------------------------
// some types
bool SceneBuilder::BuildHOMModel()
{
	CMemoryWriter F;

    F.open_chunk(0);
    F.w_u32(0);
    F.close_chunk();

    F.open_chunk(1);
    ObjectList& lst = Scene->ListObj(OBJCLASS_SCENEOBJECT);
    for (ObjectIt it=lst.begin(); it!=lst.end(); it++)
    {
    	CSceneObject* S 	= (CSceneObject*)(*it);
    	CEditableObject* E	= S->GetReference(); R_ASSERT(E);
    	if (E->m_objectFlags.is(CEditableObject::eoHOM))
        {
            Fvector 		v;
            const Fmatrix&	parent = S->_Transform();
            for (EditMeshIt m_it=E->FirstMesh(); m_it!=E->LastMesh(); ++m_it)
            {
                for (SurfFacesPairIt sf_it=(*m_it)->m_SurfFaces.begin(); sf_it!=(*m_it)->m_SurfFaces.end(); ++sf_it)
                {
                    BOOL b2Sided = sf_it->first->m_Flags.is(CSurface::sf2Sided);
                    IntVec& i_lst= sf_it->second;
                    for (IntIt i_it=i_lst.begin(); i_it!=i_lst.end(); ++i_it)
                    {
                        st_Face& face = (*m_it)->m_Faces[*i_it];
                        for (int k=0; k<3; ++k)
                        {
                            parent.transform_tiny	(v,(*m_it)->m_Vertices[face.pv[k].pindex]);
                            F.w_fvector3			(v);
                        }
                        F.w_u32						(b2Sided);
                    }
                }
            }
        }else
        {
            Fvector 		v;
            const Fmatrix&	parent = S->_Transform();
            for (EditMeshIt m_it=E->FirstMesh(); m_it!=E->LastMesh(); ++m_it)
            {
                for (SurfFacesPairIt sf_it=(*m_it)->m_SurfFaces.begin(); sf_it!=(*m_it)->m_SurfFaces.end(); ++sf_it)
                {
                	CSurface* S 	= sf_it->first;
                    if(S->m_GameMtlName=="materials\\occ")
                    {
                        BOOL b2Sided 	= S->m_Flags.is(CSurface::sf2Sided);
                        IntVec& i_lst	= sf_it->second;
                        for (IntIt i_it=i_lst.begin(); i_it!=i_lst.end(); ++i_it)
                        {
                            st_Face& face = (*m_it)->m_Faces[*i_it];
                            for (int k=0; k<3; ++k)
                            {
                                parent.transform_tiny	(v,(*m_it)->m_Vertices[face.pv[k].pindex]);
                                F.w_fvector3			(v);
                            }
                            F.w_u32						(b2Sided);
                        }
                    }
                }
            }
        }
    }
    
    BOOL bValid = !!F.chunk_size();
    F.close_chunk();
    if (bValid){
	    xr_string hom_name 	= MakeLevelPath("level.hom");
	    bValid 				= F.save_to(hom_name.c_str());
    }
	return bValid;
}

bool SceneBuilder::BuildSOMModel()
{
	BOOL bResult 	= TRUE;
	CMemoryWriter 	F;

    F.open_chunk	(0);
    F.w_u32			(0);
    F.close_chunk	();

    F.open_chunk	(1);
    ObjectList& lst = Scene->ListObj(OBJCLASS_SCENEOBJECT);
    for (ObjectIt it=lst.begin(); it!=lst.end(); it++){
    	CSceneObject* S 	= (CSceneObject*)(*it);
    	CEditableObject* E	= S->GetReference(); R_ASSERT(E);
    	if (E->m_objectFlags.is(CEditableObject::eoSoundOccluder)){ 
            Fvector 		v;
            const Fmatrix&	parent = S->_Transform();
            for (EditMeshIt m_it=E->FirstMesh(); m_it!=E->LastMesh(); m_it++){
                for (SurfFacesPairIt sf_it=(*m_it)->m_SurfFaces.begin(); sf_it!=(*m_it)->m_SurfFaces.end(); sf_it++){
                	CSurface* surf 		= sf_it->first;
                    int gm_id			= surf->_GameMtl(); 
                    if (gm_id==GAMEMTL_NONE_ID){ 
                        ELog.DlgMsg		(mtError,"Object '%s', surface '%s' contain invalid game material.",(*m_it)->Parent()->m_LibName.c_str(),surf->_Name());
                        bResult 		= FALSE; 
                        break; 
                    }
                    SGameMtl* mtl 		= GMLib.GetMaterialByID(gm_id);
                    if (0==mtl){
                        ELog.DlgMsg		(mtError,"Object '%s', surface '%s' contain undefined game material.",(*m_it)->Parent()->m_LibName.c_str(),surf->_Name());
                        bResult 		= FALSE; 
                        break; 
                    }
                    BOOL b2Sided 		= surf->m_Flags.is(CSurface::sf2Sided);
                    IntVec& i_lst		= sf_it->second;
                    for (IntIt i_it=i_lst.begin(); i_it!=i_lst.end(); i_it++){
                        st_Face& face 	= (*m_it)->m_Faces[*i_it];
                        for (int k=0; k<3; k++){
                            parent.transform_tiny(v,(*m_it)->m_Vertices[face.pv[k].pindex]);
                            F.w_fvector3(v);
                        }
                        F.w_u32			(b2Sided);
                        F.w_float		(mtl->fSndOcclusionFactor);
                    }
                }
            }
        }
    }
    BOOL bValid = !!F.chunk_size()&&bResult;
    F.close_chunk();
    if (bValid){
	    xr_string som_name 	= MakeLevelPath("level.som");
	    bValid				= F.save_to(som_name.c_str());
    }
	return bValid;
}

bool SceneBuilder::BuildAIMap()
{
	// build sky ogf
    if (Scene->GetTool(OBJCLASS_AIMAP)->Valid())
    {
        return Scene->GetTool(OBJCLASS_AIMAP)->Export(m_LevelPath);
    }
	return false;
}

bool SceneBuilder::BuildWallmarks()
{
	// build sky ogf
    if (Scene->GetTool(OBJCLASS_WM)->Valid())
    {
        return Scene->GetTool(OBJCLASS_WM)->Export(m_LevelPath);
    }
	return false;
}

