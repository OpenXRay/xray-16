#include "stdafx.h"      
#pragma hdrstop          
                              
#include "Builder.h"  
#include "ELight.h"
#include "SceneObject.h"
#include "../ECore/Editor/EditObject.h"
#include "Communicate.h"
#include "Scene.h"
#include "../ECore/Editor/EditMesh.h"
#include "../ECore/Engine/Texture.h"
#include "glow.h"
#include "sector.h"
#include "groupobject.h"
#include "portal.h"
#include "xrLevel.h"
#include "../ECore/Editor/ui_main.h"
#include "ui_leveltools.h"
#include "xrHemisphere.h"
#include "ResourceManager.h"
#include "../ECore/Editor/ImageManager.h"
#include "../ECore/Engine/Image.h"

#include "ESceneLightTools.h"

//------------------------------------------------------------------------------
// !!! использовать prefix если нужно имя !!! (Связано с группами)
//------------------------------------------------------------------------------

#define LEVEL_LODS_TEX_NAME "level_lods"
#define LEVEL_LODS_NRM_NAME "level_lods_nm"
#define LEVEL_DI_TEX_NAME "level_stat"

class CSceneStat{
	Fvector	bb_min;
	u32		bb_sx, bb_sz;

    u32 	max_svert;
	U32Vec	svertices;
    u32&	svertex(u32 ix, u32 iz){VERIFY((ix<bb_sx)&&(iz<bb_sz)); return svertices[iz*bb_sx+ix];}

    u32 	max_muvert;
	U32Vec	muvertices;
    u32&	muvertex(u32 ix, u32 iz){VERIFY((ix<bb_sx)&&(iz<bb_sz)); return muvertices[iz*bb_sx+ix];}
public:
	CSceneStat(const Fbox& bb)
    {
    	Fvector				sz;
    	bb.getsize			(sz);
    	bb_sx				= iFloor(sz.x+1.f);
        bb_sz				= iFloor(sz.z+1.f);
        bb_min.set			(bb.min);
        max_svert			= 0;
        svertices.resize	(bb_sx*bb_sz,0);
        max_muvert			= 0;
        muvertices.resize	(bb_sx*bb_sz,0);
    }
    void add_svert(const Fvector& p)
    {
    	u32 ix	= clampr((u32)iFloor(p.x-bb_min.x),(u32)0,bb_sx-1);
    	u32 iz	= clampr((u32)iFloor(p.z-bb_min.z),(u32)0,bb_sz-1);
        u32& v	= svertex(ix,iz); v++;
        if (v>max_svert) max_svert=v;
    }
    void add_muvert(const Fmatrix& parent, const Fvector& _p)
    {
    	Fvector p;
        parent.transform_tiny(p,_p);
    	u32 ix	= clampr((u32)iFloor(p.x-bb_min.x),(u32)0,bb_sx-1);
    	u32 iz	= clampr((u32)iFloor(p.z-bb_min.z),(u32)0,bb_sz-1);
        u32& v	= muvertex(ix,iz); v++;
        if (v>max_muvert) max_muvert=v;
    }
    bool flush(LPCSTR fn)
    {
    	// flush image
    	u32 sx=bb_sx, sz=bb_sz;
        U32Vec data	(sx*sz);
    	// prepare vertex info
        // find max
        u32 ix,iz;
        u32 total_svert=0;
        u32 total_muvert=0;
        for (ix=0; ix<bb_sx; ix++){
	        for (iz=0; iz<bb_sz; iz++){
            	total_svert			+= svertex(ix,iz);
            	total_muvert		+= muvertex(ix,iz);
                u8 v_s 				= (u8)iFloor(float(svertex(ix,iz))/float(max_svert)*255.f+0.5f);
                u8 v_mu 			= (u8)iFloor(float(muvertex(ix,iz))/float(max_muvert)*255.f+0.5f);
                data[iz*bb_sx+ix] 	= color_rgba(v_s,v_mu,0,0);
            }
        }
        
        AnsiString image_name = AnsiString(fn)+".tga";
        CImage* I 	= xr_new<CImage>();
        I->Create	(sx,sz,data.begin());
        I->Vflip	();
        I->SaveTGA	(image_name.c_str());
        xr_delete	(I);

        // flush text
        AnsiString txt_name = AnsiString(fn)+".txt";
        IWriter* F	= FS.w_open(txt_name.c_str());
        if (F){
            F->w_string	(AnsiString().sprintf("Map size X x Z:            [%d x %d]",bb_sx,bb_sz).c_str());
            F->w_string	(AnsiString().sprintf("Max static vertex per m^2: %d",max_svert).c_str());
            F->w_string	(AnsiString().sprintf("Total static vertices:     %d",total_svert).c_str());
            F->w_string	(AnsiString().sprintf("Max mu vertex per m^2:     %d",max_muvert).c_str());
            F->w_string	(AnsiString().sprintf("Total mu vertices:         %d",total_muvert).c_str());
            FS.w_close	(F);
	        return true;
        }
        return false;
    }
};

void SceneBuilder::SaveBuildAsObject()
{
    string512 				tmp, tex_path, tex_name;
    xr_string 				fn, fn_material;

    if (! EFS.GetSaveName	( _import_, fn ))
    	return;

    fn 				= EFS.ChangeFileExt(fn,".obj");
    fn_material 	= EFS.ChangeFileExt(fn,".mtl");

	IWriter* Fm		= FS.w_open(fn_material.c_str());

    for(u32 i=0;i<l_materials.size(); ++i)
    {
    	b_material& m		= l_materials[i];
        b_texture&	t 		= l_textures[m.surfidx];
	    _splitpath			(t.name, 0, tex_path, tex_name, 0 );

        sprintf				(tmp,"newmtl %s", tex_name);
		Fm->w_string		(tmp);
		Fm->w_string		("Ka  0 0 0");
		Fm->w_string		("Kd  1 1 1");
		Fm->w_string		("Ks  0 0 0");

        sprintf				(tmp,"map_Kd %s\\\\%s\\%s%s\n",
        									"T:",
                                            tex_path,
                                            tex_name,
                                            ".tga");
		Fm->w_string		(tmp);
    }
    FS.w_close		(Fm);

	IWriter* F				= FS.w_open(fn.c_str());
	CMemoryWriter			tmpFaces;

	// writ comment
    F->w_string				("# This file uses meters as units for non-parametric coordinates.");
    _splitpath				(fn.c_str(), 0, 0, tex_name, 0 );
    sprintf					(tmp,"mtllib %s.mtl", tex_name);
    F->w_string				(tmp);

    F->w_string				("g default");

	//vertices
	u32						idx;
	u32						total_vertices = 0;
	u32						total_tcs = 0;
	for(idx=0; idx<l_vert_it; ++idx)
	{
		const b_vertex& it	= l_verts[idx];
        sprintf				(tmp,"v %f %f %f",it.x*100.0f, it.y*100.0f, it.z*100.0f);
		F->w_string			(tmp);

	}

	//TC-s
	for(idx=0; idx<l_face_it; ++idx)
	{
		const b_face& it	= l_faces[idx];
        sprintf				(tmp,"vt %f %f", it.t[0].x, /*_abs*/(1.f-it.t[0].y));
		tmpFaces.w_string	(tmp);
        sprintf				(tmp,"vt %f %f", it.t[1].x, /*_abs*/(1.f-it.t[1].y));
		tmpFaces.w_string	(tmp);
        sprintf				(tmp,"vt %f %f", it.t[2].x, /*_abs*/(1.f-it.t[2].y));
		tmpFaces.w_string	(tmp);
	}
    total_tcs				+= idx*3;

	//faces
    b_texture* last_texture = NULL;
	for(idx=0; idx<l_face_it; ++idx)
	{
		const b_face& it			= l_faces[idx];

        b_material& m				= l_materials[it.dwMaterial];
        b_texture&	t 				= l_textures[m.surfidx];
        if(last_texture != &t)
        {
	    	_splitpath			(t.name, 0, 0, tex_name, 0 );
            sprintf				(tmp,"usemtl %s", tex_name);
            tmpFaces.w_string	(tmp);
        	last_texture 		= &t;
        }

        sprintf				(tmp,"f %d/%d %d/%d %d/%d", it.v[0]+1, idx*3+1,
        												it.v[1]+1, idx*3+2,
                                                        it.v[2]+1, idx*3+3);
		tmpFaces.w_string	(tmp);
	}
	total_vertices += l_vert_it;

    for (idx=0; idx<l_mu_models.size(); ++idx)
	{
        const b_mu_model&	m = l_mu_models[idx];

		for(u32 vi=0; vi<m.m_iVertexCount; ++vi)
		{
			const b_vertex& it	= m.m_pVertices[vi];
			sprintf				(tmp,"v %f %f %f",it.x*100.0f, it.y*100.0f, it.z*100.0f);
			F->w_string			(tmp);
		}
        //TC-s
		for(u32 fi=0; fi<m.m_iFaceCount; ++fi)
		{
			const b_face& it	= m.m_pFaces[fi];
            sprintf				(tmp,"vt %f %f", it.t[0].x, /*_abs*/(1.f-it.t[0].y));
            tmpFaces.w_string	(tmp);
            sprintf				(tmp,"vt %f %f", it.t[1].x, /*_abs*/(1.f-it.t[1].y));
            tmpFaces.w_string	(tmp);
            sprintf				(tmp,"vt %f %f", it.t[2].x, /*_abs*/(1.f-it.t[2].y));
            tmpFaces.w_string	(tmp);
		}
        //faces
		for(fi=0; fi<m.m_iFaceCount; ++fi)
		{
			const b_face& it		= m.m_pFaces[fi];

            b_material& m			= l_materials[it.dwMaterial];
            b_texture&	t 			= l_textures[m.surfidx];
            if(last_texture != &t)
            {
                _splitpath			(t.name, 0, 0, tex_name, 0 );
                sprintf				(tmp,"usemtl %s", tex_name);
                tmpFaces.w_string	(tmp);
                last_texture 		= &t;
			}
        	sprintf			(tmp,"f %d/%d %d/%d %d/%d", it.v[0]+1+total_vertices, fi*3+1+total_tcs,
        												it.v[1]+1+total_vertices, fi*3+2+total_tcs,
                                                        it.v[2]+1+total_vertices, fi*3+3+total_tcs);
/*
			sprintf				(tmp,"f %d %d %d",	it.v[0]+1+total_vertices,
													it.v[1]+1+total_vertices,
													it.v[2]+1+total_vertices );
*/
			tmpFaces.w_string	(tmp);
		}
        total_tcs				+= m.m_iFaceCount*3;
		total_vertices += m.m_iVertexCount;
    }
	F->w(tmpFaces.pointer(),tmpFaces.size());

	//uv
//                sprintf			(tmp,"vt %f %f",v_it->UV.x,_abs(1.f-v_it->UV.y));		F.w_string	(tmp);

	//normals
//                sprintf			(tmp,"vn %f %f %f",mV.x,mV.y,mV.z);		F.w_string	(tmp);

	//g
//                sprintf			(tmp,"vg %f %f %f",mV.x,mV.y,mV.z);		F.w_string	(tmp);

	//b
//                sprintf			(tmp,"vb %f %f %f",mV.x,mV.y,mV.z);		F.w_string	(tmp);




/*
    string512 	tmp,tex_path,tex_name;
    // write mtl
    for (SplitIt split_it=m_Splits.begin(); split_it!=m_Splits.end(); split_it++)
	{
	    _splitpath			((*split_it)->m_Surf->_Texture(), 0, 0, tex_name, 0 );
    	sprintf				(tmp,"newmtl %s",tex_name);
		F.w_string			(tmp);

	    _splitpath			((*split_it)->m_Surf->_Texture(), 0, tex_path, tex_name, 0 );
        strconcat			(sizeof(tex_path),tex_path,tex_path,"\\",tex_name,".tga");
    	sprintf				(tmp,"map_Kd %s",tex_path);
		F.w_string	(tmp);
    }
    sprintf					(tmp,"mtllib %s",name);
	F.w_string				(tmp);

    // write mtl
    u32 v_offs				= 0;
    for (split_it=m_Splits.begin(); split_it!=m_Splits.end(); split_it++){
	    _splitpath			((*split_it)->m_Surf->_Texture(), 0, 0, tex_name, 0 );
        sprintf				(tmp,"g %d",split_it-m_Splits.begin());				F.w_string	(tmp);
        sprintf				(tmp,"usemtl %s",tex_name);							F.w_string	(tmp);
        Fvector 			mV;
        Fmatrix 			mZ;
        mZ.mirrorZ			();
        for (COGFCPIt it=(*split_it)->m_Parts.begin(); it!=(*split_it)->m_Parts.end(); it++){
            CObjectOGFCollectorPacked* part = *it;
            // vertices
            OGFVertVec& VERTS	= part->getV_Verts();
            OGFVertIt 			v_it;
            for (v_it=VERTS.begin(); v_it!=VERTS.end(); v_it++){
            	mZ.transform_tiny(mV,v_it->P);
                sprintf			(tmp,"v %f %f %f",mV.x,mV.y,mV.z); 		F.w_string	(tmp);
            }
            for (v_it=VERTS.begin(); v_it!=VERTS.end(); v_it++){
                sprintf			(tmp,"vt %f %f",v_it->UV.x,_abs(1.f-v_it->UV.y));		F.w_string	(tmp);
            }
            for (v_it=VERTS.begin(); v_it!=VERTS.end(); v_it++){
            	mZ.transform_dir(mV,v_it->N);
                sprintf			(tmp,"vn %f %f %f",mV.x,mV.y,mV.z);		F.w_string	(tmp);
            }
            for (v_it=VERTS.begin(); v_it!=VERTS.end(); v_it++){
            	mZ.transform_dir(mV,v_it->T);
                sprintf			(tmp,"vg %f %f %f",mV.x,mV.y,mV.z);		F.w_string	(tmp);
            }
            for (v_it=VERTS.begin(); v_it!=VERTS.end(); v_it++){
            	mZ.transform_dir(mV,v_it->B);
                sprintf			(tmp,"vb %f %f %f",mV.x,mV.y,mV.z);		F.w_string	(tmp);
            }
            // faces
            OGFFaceVec& FACES	= part->getV_Faces();
            OGFFaceIt 			f_it;
            for (f_it=FACES.begin(); f_it!=FACES.end(); f_it++){
                sprintf			(tmp,"f %d/%d/%d %d/%d/%d %d/%d/%d",v_offs+f_it->v[2]+1,v_offs+f_it->v[2]+1,v_offs+f_it->v[2]+1,
                                                                    v_offs+f_it->v[1]+1,v_offs+f_it->v[1]+1,v_offs+f_it->v[1]+1,
                                                                    v_offs+f_it->v[0]+1,v_offs+f_it->v[0]+1,v_offs+f_it->v[0]+1); 	F.w_string	(tmp);
            }
	        v_offs  			+= VERTS.size();
        }
    }
}
*/
//---
    FS.w_close		(F);
}

void SceneBuilder::SaveBuild()
{
    xr_string fn	= MakeLevelPath("build.prj");
	IWriter* F		= FS.w_open(fn.c_str());
    if (F){
        F->open_chunk	(EB_Version);
        F->w_u32   		(XRCL_CURRENT_VERSION);
        F->close_chunk	();

        F->open_chunk	(EB_Parameters);
        F->w	  		(&Scene->m_LevelOp.m_BuildParams,sizeof(b_params));
        F->close_chunk	();

        F->open_chunk	(EB_Vertices);
        F->w		  	(l_verts,sizeof(b_vertex)*l_vert_it); 	//. l_vert_cnt
        F->close_chunk	();

        F->open_chunk	(EB_Faces);
        F->w		  	(l_faces,sizeof(b_face)*l_face_it); 	//. l_face_cnt
        F->close_chunk	();

        F->open_chunk	(EB_SmoothGroups);
        F->w		  	(l_smgroups, sizeof(u32)*l_face_it); 	//. l_face_cnt
        F->close_chunk	();

        F->open_chunk	(EB_Materials);
        F->w	   		(l_materials.begin(),sizeof(b_material)*l_materials.size());
        F->close_chunk	();

        F->open_chunk	(EB_Shaders_Render);
        F->w			(l_shaders.begin(),sizeof(b_shader)*l_shaders.size());
        F->close_chunk	();

        F->open_chunk	(EB_Shaders_Compile);
        F->w			(l_shaders_xrlc.begin(),sizeof(b_shader)*l_shaders_xrlc.size());
        F->close_chunk	();

        F->open_chunk	(EB_Textures);
        F->w			(l_textures.begin(),sizeof(b_texture)*l_textures.size());
        F->close_chunk	();

        F->open_chunk 	(EB_Glows);
        F->w			(l_glows.begin(),sizeof(b_glow)*l_glows.size());
        F->close_chunk	();

        F->open_chunk	(EB_Portals);
        F->w			(l_portals.begin(),sizeof(b_portal)*l_portals.size());
        F->close_chunk	();

        F->open_chunk	(EB_Light_control);
        for (xr_vector<sb_light_control>::iterator lc_it=l_light_control.begin(); lc_it!=l_light_control.end(); lc_it++){
            F->w		(lc_it->name,sizeof(lc_it->name));
            F->w_u32 	(lc_it->data.size());
            F->w	 	(lc_it->data.begin(),sizeof(u32)*lc_it->data.size());
        }
        F->close_chunk	();

        F->open_chunk	(EB_Light_static);
        F->w		 	(l_light_static.begin(),sizeof(b_light_static)*l_light_static.size());
        F->close_chunk	();

        F->open_chunk	(EB_Light_dynamic);
        F->w		  	(l_light_dynamic.begin(),sizeof(b_light_dynamic)*l_light_dynamic.size());
        F->close_chunk	();

        F->open_chunk	(EB_LOD_models);
        for (int k=0; k<(int)l_lods.size(); ++k)
            F->w	  	(&l_lods[k].lod,sizeof(b_lod));
        F->close_chunk	();

        F->open_chunk	(EB_MU_models);
        for (k=0; k<(int)l_mu_models.size(); ++k)
        {
            b_mu_model&	m= l_mu_models[k];
            // name
            F->w_stringZ(m.name);
            // vertices
            F->w_u32	(m.m_iVertexCount);
            F->w		(m.m_pVertices,sizeof(b_vertex)*m.m_iVertexCount);
            // faces
            F->w_u32	(m.m_iFaceCount);
            F->w		(m.m_pFaces,sizeof(b_face)*m.m_iFaceCount);
            // lod_id
            F->w_u16	(m.lod_id);
            F->w		(m.m_smgroups,sizeof(int)*m.m_iFaceCount);
        }
        F->close_chunk	();

        F->open_chunk	(EB_MU_refs);
        F->w			(l_mu_refs.begin(),sizeof(b_mu_reference)*l_mu_refs.size());
        F->close_chunk	();

        FS.w_close		(F);
    }
}

int SceneBuilder::CalculateSector(const Fvector& P, float R)
{
    ObjectIt _F = Scene->FirstObj(OBJCLASS_SECTOR);
    ObjectIt _E = Scene->LastObj(OBJCLASS_SECTOR);
    for(;_F!=_E;_F++){
    	CSector* _S=(CSector*)(*_F);
        EVisible vis=_S->Intersect(P,R);
        if ((vis==fvPartialInside)||(vis==fvFully))
        	if (_S->m_sector_num!=m_iDefaultSectorNum) 
            	return _S->m_sector_num;
	}
    return m_iDefaultSectorNum; // по умолчанию
}

void SceneBuilder::Clear ()
{
    object_for_render		= 0;
    l_vert_cnt 				= 0;
	l_face_cnt				= 0;
	l_vert_it 				= 0;
	l_face_it				= 0;
    xr_free					(l_verts);
    xr_free					(l_faces);
    xr_free					(l_smgroups);

    for (int k=0; k<(int)l_mu_models.size(); k++){
    	b_mu_model&	m 		= l_mu_models[k];
        xr_free				(m.m_pVertices);
        xr_free				(m.m_pFaces);
        xr_free				(m.m_smgroups);
    }
    l_mu_models.clear		();
    l_mu_refs.clear			();
    l_lods.clear			();
    l_light_static.clear	();
    l_light_dynamic.clear	();
    l_light_control.clear	();
    l_textures.clear		();
    l_shaders.clear			();
    l_shaders_xrlc.clear	();
    l_materials.clear		();
    l_vnormals.clear		();
    l_glows.clear			();
    l_portals.clear			();
    l_light_keys.clear		();
    xr_delete				(l_scene_stat);
}

//------------------------------------------------------------------------------
// CEditObject build functions
//------------------------------------------------------------------------------
float CalcArea(const Fvector& v0, const Fvector& v1, const Fvector& v2)
{
	float	e1 = v0.distance_to(v1);
	float	e2 = v0.distance_to(v2);
	float	e3 = v1.distance_to(v2);

	float	p  = (e1+e2+e3)/2.f;
	return	_sqrt( p*(p-e1)*(p-e2)*(p-e3) );
}


//const Fmatrix& parent,
BOOL  GetStaticCformData( const Fmatrix& parent,   CEditableMesh* mesh, CEditableObject* object, Fvector*  verts, int& vert_cnt, int& vert_it, CDB::TRI *faces , int& face_cnt,  int& face_it )
{

	if (object->IsDynamic())
    	return FALSE;
    //Fmatrix parent 			= object->_Transform();
    
     BOOL bResult = TRUE;
     int point_offs = vert_it;  // save offset
     // fill vertices
	 for (u32 pt_id=0; pt_id<mesh->GetVCount(); pt_id++){
    	R_ASSERT(vert_it<vert_cnt);
    	parent.transform_tiny(verts[vert_it++],mesh->Vertices()[pt_id]);
    }
    for (SurfFacesPairIt sp_it=mesh->Surfaces().begin(); sp_it!=mesh->Surfaces().end(); sp_it++)
    {
		IntVec& face_lst = sp_it->second;
        CSurface* surf 		= sp_it->first;
        if(surf->m_GameMtlName=="materials\\occ")
			continue;

        u16 game_material_idx = GMLib.GetMaterialIdx(surf->m_GameMtlName.c_str());

        for (IntIt f_it=face_lst.begin(); f_it!=face_lst.end(); ++f_it)
        {
			st_Face& face = mesh->Faces()[*f_it];
            float _a		= CalcArea(mesh->Vertices()[face.pv[0].pindex],mesh->Vertices()[face.pv[1].pindex],mesh->Vertices()[face.pv[2].pindex]);
	    	if (!_valid(_a) || (_a<EPS))
            {
                continue;
            }
            R_ASSERT				(face_it<face_cnt);
            CDB::TRI& first_face 	 = faces[face_it];
                 	{

                first_face.material 		= (u16)game_material_idx;
                
                
                for (int k=0; k<3; ++k)
                {
                    st_FaceVert& fv = face.pv[k];
                    // vertex index
                    R_ASSERT2((fv.pindex+point_offs)<vert_it,"Index out of range");
                    first_face.verts[k] = fv.pindex+point_offs;
                    // uv maps

                }
               ++face_it;
               if (surf->m_Flags.is(CSurface::sf2Sided))
               {
                  R_ASSERT					(face_it<face_cnt);
                  CDB::TRI&  second_face 		= faces[face_it];
                  second_face.material 		= first_face.material;
                  for (int k=0; k<3; ++k)
                  {
                      st_FaceVert& fv 			= face.pv[2-k];
                      // vertex index
                      second_face.verts[k]		=fv.pindex+point_offs;
                      // uv maps
                  }
                  ++face_it;
             }
			}
          }
       }
       return bResult;
}





BOOL SceneBuilder::BuildMesh(	const Fmatrix& parent,
								CEditableObject* object,
                                CEditableMesh* mesh,
                                int sect_num,
								b_vertex* verts,
                                int& vert_cnt,
                                int& vert_it,
                                b_face* faces,
                                int& face_cnt,
                                int& face_it,
                                u32* smgroups,
                                const Fmatrix& real_transform)
{
	BOOL bResult = TRUE;
    int point_offs;
    point_offs = vert_it;  // save offset

    // fill vertices
	for (u32 pt_id=0; pt_id<mesh->GetVCount(); pt_id++){
    	R_ASSERT(vert_it<vert_cnt);
    	parent.transform_tiny(verts[vert_it++],mesh->m_Vertices[pt_id]);
    }

    if (object->IsDynamic())
	{
	    // update mesh
	    mesh->GenerateFNormals();
	    mesh->GenerateAdjacency();
		Fvector N;
		for (u32 pt=0; pt<mesh->GetVCount(); pt++)
		{
            N.set(0,0,0);
            IntVec& a_lst = (*mesh->m_Adjs)[pt];
            VERIFY(a_lst.size());
            for (IntIt i_it=a_lst.begin(); i_it!=a_lst.end(); i_it++)
                N.add((*mesh->m_FaceNormals)[*i_it]);
            N.normalize_safe();
            parent.transform_dir(N);
            l_vnormals.push_back(N);
        }
	    // unload mesh normals
	    mesh->UnloadAdjacency();
	    mesh->UnloadFNormals();
    }
    // fill faces
    for (SurfFacesPairIt sp_it=mesh->m_SurfFaces.begin(); sp_it!=mesh->m_SurfFaces.end(); sp_it++)
    {
		IntVec& face_lst = sp_it->second;
        CSurface* surf 		= sp_it->first;
        if(surf->m_GameMtlName=="materials\\occ")
			continue;
                    	                       	
        int m_id			= BuildMaterial(surf,sect_num,!object->IsMUStatic());
		int gm_id			= surf->_GameMtl();
        if (m_id<0)			{
        	bResult = FALSE;
            break;
        }
        if (gm_id<0)
        {
        	ELog.DlgMsg		(mtError,"Surface: '%s' contains bad game material.",surf->_Name());
        	bResult 		= FALSE;
            break;
        }
        SGameMtl* M = GMLib.GetMaterialByID(gm_id);
        if (0==M)
        {
        	ELog.DlgMsg		(mtError,"Surface: '%s' contains undefined game material.",surf->_Name());
        	bResult 		= FALSE;
            break;
        }
        if (M->Flags.is(SGameMtl::flBreakable))
        {
        	ELog.Msg		(mtInformation,"Surface: '%s' contains breakable game material.",surf->_Name());
            continue;
        }
        if (M->Flags.is(SGameMtl::flClimable))
        {
        	ELog.Msg		(mtInformation,"Surface: '%s' contains climable game material.",surf->_Name());
            continue;
        }
        if (M->Flags.is(SGameMtl::flDynamic))
        {
        	ELog.DlgMsg		(mtError,"Surface: '%s' contains non-static game material.",surf->_Name());
        	bResult 		= FALSE;
            break;
        }
		u32 dwTexCnt 		= ((surf->_FVF()&D3DFVF_TEXCOUNT_MASK)>>D3DFVF_TEXCOUNT_SHIFT);
        if (dwTexCnt!=1)
        {
        	ELog.DlgMsg		(mtError,"Surface: '%s' contains more than 1 texture refs.",surf->_Name());
        	bResult 		= FALSE; 
            break; 
        }
        u32 dwInvalidFaces 	= 0;
	    for (IntIt f_it=face_lst.begin(); f_it!=face_lst.end(); ++f_it)
        {
			st_Face& face = mesh->m_Faces[*f_it];
            float _a		= CalcArea(mesh->m_Vertices[face.pv[0].pindex],mesh->m_Vertices[face.pv[1].pindex],mesh->m_Vertices[face.pv[2].pindex]);
	    	if (!_valid(_a) || (_a<EPS))
            {
            	Fvector p0,p1,p2;

    			real_transform.transform_tiny(p0,mesh->m_Vertices[face.pv[0].pindex]);
    			real_transform.transform_tiny(p1,mesh->m_Vertices[face.pv[1].pindex]);
    			real_transform.transform_tiny(p2,mesh->m_Vertices[face.pv[2].pindex]);
            	Tools->m_DebugDraw.AppendWireFace(p0,p1,p2);

            	dwInvalidFaces++;
                continue;
            }
            R_ASSERT				(face_it<face_cnt);
            b_face& first_face 		= faces[face_it];
        	{
                smgroups[face_it]			= mesh->m_SmoothGroups[*f_it];
				smgroups[face_it]			&= ~(1<<3);

                first_face.dwMaterial 		= (u16)m_id;
                first_face.dwMaterialGame 	= gm_id;
                for (int k=0; k<3; ++k)
                {
                    st_FaceVert& fv = face.pv[k];
                    // vertex index
                    R_ASSERT2((fv.pindex+point_offs)<vert_it,"Index out of range");
                    first_face.v[k] = fv.pindex+point_offs;
                    // uv maps
                    int offs = 0;
                    for (u32 t=0; t<dwTexCnt; ++t)
                    {
                        st_VMapPt& vm_pt 	= mesh->m_VMRefs[fv.vmref].pts[t];
                        st_VMap& vmap		= *mesh->m_VMaps[vm_pt.vmap_index];
                        if (vmap.type!=vmtUV)
                        {
                            ++offs;
                            --t;
                            continue;
                        }
                        first_face.t[k].set(vmap.getUV(vm_pt.index));
                    }
                }
            ++face_it;
            }

	        if (surf->m_Flags.is(CSurface::sf2Sided))
            {
		    	R_ASSERT					(face_it<face_cnt);
                b_face& second_face 		= faces[face_it];
                second_face.dwMaterial 		= first_face.dwMaterial;
                second_face.dwMaterialGame 	= first_face.dwMaterialGame;
                smgroups[face_it]			= mesh->m_SmoothGroups[*f_it];
				smgroups[face_it]			|= (1<<3);

                for (int k=0; k<3; ++k)
                {
                    st_FaceVert& fv 		= face.pv[2-k];
                    // vertex index
                    second_face.v[k]		=fv.pindex+point_offs;
                    // uv maps
                    int offs = 0;
                    for (u32 t=0; t<dwTexCnt; t++)
                    {
                        st_VMapPt& vm_pt 	= mesh->m_VMRefs[fv.vmref].pts[t];
                        st_VMap& vmap		= *mesh->m_VMaps[vm_pt.vmap_index];
                        if (vmap.type!=vmtUV)
                        {
                            ++offs;
                            --t;
                            continue;
                        }
                        second_face.t[k].set(vmap.getUV(vm_pt.index));
                    }
                }
                ++face_it;
            }
        }
        if (dwInvalidFaces)
        	Msg("!Object '%s' - '%s' has %d invalid face(s). Removed.",object->GetName(),mesh->Name().c_str(),dwInvalidFaces);

        if (!bResult)
        	break;
    }
    return bResult;
}

BOOL SceneBuilder::BuildObject(CSceneObject* obj)
{
	CEditableObject *O = obj->GetReference();
    AnsiString temp;
    temp.sprintf("Building object: %s",obj->Name);
    UI->SetStatus(temp.c_str());

    Fmatrix T 			= obj->_Transform();
	
	Fmatrix cv 			= Fidentity;

	if(m_save_as_object)
	{
		cv.k.z 			= -1.f;

		Fmatrix 	TM;

        TM.mul		( Fmatrix().mul(cv,T), cv );
        TM.mulB_44	( cv );
		T 			= TM;
	}

	// parse mesh data
    for(EditMeshIt M=O->FirstMesh();M!=O->LastMesh();M++){
		CSector* S = PortalUtils.FindSector(obj,*M);
	    int sect_num = S?S->m_sector_num:m_iDefaultSectorNum;
    	if (!BuildMesh(T,O,*M,sect_num,l_verts,l_vert_cnt,l_vert_it,l_faces,l_face_cnt,l_face_it,l_smgroups,obj->_Transform()))
        	return FALSE;
        // fill DI vertices
        for (u32 pt_id=0; pt_id<(*M)->GetVCount(); pt_id++)
		{
        	Fvector						v_res1, v_res2;
        	const Fvector&	v_src 		= (*M)->m_Vertices[pt_id];

            Fvector 			tmp;
            cv.transform_tiny	( tmp , 	v_src );
            T.transform_tiny	( v_res1, 	tmp );

          	l_scene_stat->add_svert(v_res1);
        }
    }
    return TRUE;
}

int	GetModelIdx( LPCSTR model_name )
{
	int model_idx		= -1;

    for (int k=0; k<(int)Builder.l_mu_models.size(); k++){
    	b_mu_model&	m 	= Builder.l_mu_models[k];
    	if (0==strcmp(m.name,model_name)){
        	model_idx	= k;
            break;
        }
    }
    return   model_idx;
}

//BOOL GetMuStaticCformData( CSceneObject* obj,mesh_build_data &data, bool b_selected_only )
//{
//   CEditableObject *O = obj->GetReference();
//   int model_idx		= GetModelIdx(O->GetName());
     // detect sector
//    CSector* S 			= PortalUtils.FindSector(obj,*O->FirstMesh());
//    int sect_num 		= S?S->m_sector_num:Builder.m_iDefaultSectorNum;
//}



BOOL SceneBuilder::BuildMUObject(CSceneObject* obj)
{
	CEditableObject *O = obj->GetReference();
    AnsiString temp; temp.sprintf("Building object: %s",obj->Name);
    UI->SetStatus(temp.c_str());

    int model_idx = GetModelIdx( O->GetName() ) ;

    // detect sector
    CSector* S 			= PortalUtils.FindSector(obj,*O->FirstMesh());
    int sect_num 		= S?S->m_sector_num:m_iDefaultSectorNum;

    // build model
    if (-1==model_idx || m_save_as_object)
    {
	    // build LOD
        int	lod_id 		= BuildObjectLOD(Fidentity,O,sect_num);
        if (lod_id==-2) return FALSE;
        // build model
        model_idx		= l_mu_models.size();
	    l_mu_models.push_back(b_mu_model());
		b_mu_model&	M	= l_mu_models.back();
        M.lod_id		= (u16)lod_id;
        int vert_it=0, face_it=0;

        M.m_iFaceCount		= obj->GetFaceCount();
        M.m_iVertexCount	= obj->GetVertexCount();
        strcpy			(M.name,O->GetName());

        M.m_pFaces				= xr_alloc<b_face>(M.m_iFaceCount);
        M.m_pVertices			= xr_alloc<b_vertex>(M.m_iVertexCount);
        M.m_smgroups			= xr_alloc<u32>(M.m_iFaceCount);
		// parse mesh data
		Fmatrix T;
		T.identity();

		if(m_save_as_object)
		{
			T 					= obj->_Transform();
			
			Fmatrix cv 			= Fidentity;

			cv.k.z 				= -1.f;

			Fmatrix 			TM;

			TM.mul				( Fmatrix().mul(cv,T), cv );
			TM.mulB_44			( cv );
			T 					= TM;
		}

	    for(EditMeshIt MESH=O->FirstMesh();MESH!=O->LastMesh();++MESH)
	    	if (!BuildMesh(T, O, *MESH, sect_num, M.m_pVertices, M.m_iVertexCount, vert_it, M.m_pFaces, M.m_iFaceCount, face_it, M.m_smgroups, obj->_Transform()))
            	return FALSE;

        M.m_iFaceCount			= face_it;
        M.m_iVertexCount		= vert_it;
    }

    l_mu_refs.push_back	(b_mu_reference());
	b_mu_reference&	R	= l_mu_refs.back();
    R.model_index		= model_idx;
    R.transform			= obj->_Transform();
    R.flags.zero		();
	R.sector			= (u16)sect_num;

    // scene statssm
    b_mu_model& M		= l_mu_models[model_idx];

    for (u32 mu_vi=0; mu_vi<(u32)M.m_iVertexCount; ++mu_vi)
    	l_scene_stat->add_muvert(obj->_Transform(),M.m_pVertices[mu_vi]);
    
    return TRUE;
}

//------------------------------------------------------------------------------
// light build functions
//------------------------------------------------------------------------------
int	SceneBuilder::BuildLightControl(LPCSTR name)
{
	for (u32 k=0; k<l_light_control.size(); k++){
    	sb_light_control& b = l_light_control[k];
    	if (0==strcmp(b.name,name)) return k;
    }
    l_light_control.push_back(sb_light_control());
    sb_light_control& b = l_light_control.back();
    strcpy(b.name,name);
	return l_light_control.size()-1;
}

struct SHemiData
{
    SceneBuilder::BLVec* 		dest;
    SceneBuilder::SBuildLight	T;
};
void __stdcall 	hemi_callback(float x, float y, float z, float E, LPVOID P)
{
    SHemiData*	H		= (SHemiData*)P;
    H->T.energy     	= E;
    H->T.light.direction.set(x,y,z);
    H->dest->push_back  (H->T);
}
void SceneBuilder::BuildHemiLights(u8 quality, LPCSTR lcontrol)
{
    BLVec 				dest;
    Flight				RL;
    // set def params
    RL.type				= D3DLIGHT_DIRECTIONAL;
    RL.diffuse.set		(1.f,1.f,1.f,1.f);
    if (quality){
        SHemiData		h_data;
        h_data.dest 	= &dest;
        h_data.T.light	= RL;
        xrHemisphereBuild(quality,2.f,hemi_callback,&h_data);   //. hack
        int control_ID	= BuildLightControl(lcontrol);
        for (BLIt it=dest.begin(); it!=dest.end(); it++){
            l_light_static.push_back(b_light_static());
            b_light_static& sl	= l_light_static.back();
            sl.controller_ID 	= control_ID;
            sl.data			    = it->light;
            sl.data.mul			(it->energy);
        }
    }else{
        int control_ID		= BuildLightControl(lcontrol);
        l_light_static.push_back(b_light_static());
        b_light_static& sl	= l_light_static.back();
        sl.controller_ID 	= control_ID;
	    sl.data.type			= D3DLIGHT_DIRECTIONAL;
    	sl.data.diffuse.set		(1.f,1.f,1.f,1.f);
        sl.data.direction.set	(0.f,-1.f,0.f);
    }
}
BOOL SceneBuilder::BuildSun(u8 quality, Fvector2 dir)
{
    int controller_ID		= BuildLightControl(LCONTROL_SUN);
    // static
    // soft light
    int samples;
    switch(quality){
    case 0: samples = 1; break;
    case 1: samples = 3; break;
    case 2: samples = 4; break;
    case 3: samples = 7; break;
    default:
        THROW2("Invalid case.");
    }

    Fcolor color;		color.set(1.f,1.f,1.f,1.f);
    float sample_energy	= 1.f/float(samples*samples);
    color.mul_rgb		(sample_energy);

    float disp			= deg2rad(3.f); // dispersion of sun
    float da 			= disp/float(samples);
    float mn_x  		= dir.x-disp/2;
    float mn_y  		= dir.y-disp/2;
    for (int x=0; x<samples; x++){
        float x = mn_x+x*da;
        for (int y=0; y<samples; y++){
            float y = mn_y+y*da;
            l_light_static.push_back(b_light_static());
            b_light_static& sl	= l_light_static.back();
            sl.controller_ID 	= controller_ID;
            sl.data.type		= D3DLIGHT_DIRECTIONAL;
            sl.data.position.set(0,0,0);
            sl.data.diffuse.set	(color);
            sl.data.direction.setHP(y,x);
        }
    }
    // dynamic
    {
		l_light_dynamic.push_back(b_light_dynamic());
        b_light_dynamic& dl	= l_light_dynamic.back();
        dl.controller_ID 	= controller_ID;
        dl.data.type		= D3DLIGHT_DIRECTIONAL;
        dl.data.position.set(0,0,0);
        dl.data.diffuse.set	(1.f,1.f,1.f,1.f);
        dl.data.direction.setHP(dir.y,dir.x);
    }

	return TRUE;
}

BOOL SceneBuilder::BuildPointLight(b_light* b, const Flags32& usage, svector<WORD,16>* sectors, FvectorVec* soft_points, const Fmatrix* soft_transform)
{
    if (usage.is(ELight::flAffectStatic)){
    	if (soft_points&&!soft_points->empty()){
        	R_ASSERT(soft_transform);
        // make soft light
            Fcolor color		= b->data.diffuse;
            color.normalize_rgb	(b->data.diffuse);
            float sample_energy	= (b->data.diffuse.magnitude_rgb())/float(soft_points->size());
            color.mul_rgb		(sample_energy);

            for (u32 k=0; k<soft_points->size(); k++){
                l_light_static.push_back(b_light_static());
                b_light_static& sl	= l_light_static.back();
                sl.controller_ID 	= b->controller_ID;
                sl.data				= b->data;
                sl.data.diffuse.set	(color);
                soft_transform->transform_tiny(sl.data.position,(*soft_points)[k]);
            }
        }else{
	        // make single light
            l_light_static.push_back(b_light_static());
            b_light_static& sl	= l_light_static.back();
            sl.controller_ID 	= b->controller_ID;
            sl.data			    = b->data;
        }
    }
    if (usage.is(ELight::flAffectDynamic)){
        R_ASSERT			(sectors);
		l_light_dynamic.push_back(b_light_dynamic());
        b_light_dynamic& dl	= l_light_dynamic.back();
        dl.controller_ID 	= b->controller_ID;
        dl.data			    = b->data;
        dl.sectors			= *sectors;
    }

	return TRUE;
}

BOOL SceneBuilder::BuildLight(CLight* e)
{
    if (!e->m_Flags.is_any(ELight::flAffectStatic|ELight::flAffectDynamic))
    	return FALSE;

    if (!e->GetLControlName()){
    	ELog.Msg(mtError,"Invalid light control name: light '%s'.",e->Name);
    	return FALSE;
    }
        
    b_light	L;
    L.data.type					= e->m_Type;
    L.data.diffuse.mul_rgb		(e->m_Color,e->m_Brightness);
    L.data.position.set			(e->PPosition);
    Fvector dir;    dir.setHP	(e->PRotation.y,e->PRotation.x);
    L.data.direction.set		(dir);
    L.data.range				= e->m_Range;
    L.data.attenuation0			= e->m_Attenuation0;
    L.data.attenuation1			= e->m_Attenuation1;
    L.data.attenuation2			= e->m_Attenuation2;
    L.data.phi					= e->m_Cone;
    
    L.controller_ID	= BuildLightControl(e->GetLControlName()); //BuildLightControl(LCONTROL_STATIC); 

	svector<u16,16>* lpSectors;
    if (e->m_Flags.is(ELight::flAffectDynamic)){
		svector<u16,16> sectors;
        lpSectors		= &sectors;
        Fvector pos 	= e->PPosition;
        float& range 	= e->m_Range;
        if (Scene->ObjCount(OBJCLASS_SECTOR)){
            // test fully and partial inside
            ObjectIt _F = Scene->FirstObj(OBJCLASS_SECTOR);
            ObjectIt _E = Scene->LastObj(OBJCLASS_SECTOR);
            for(;_F!=_E;_F++){
                if (sectors.size()>=16) break;
                CSector* _S=(CSector*)(*_F);
                EVisible vis=_S->Intersect(pos,range);
                if ((vis==fvPartialInside)||(vis==fvFully))
                    sectors.push_back((u16)_S->m_sector_num);
            }
            // test partial outside
            _F = Scene->FirstObj(OBJCLASS_SECTOR);
            for(;_F!=_E;_F++){
                if (sectors.size()>=16) break;
                CSector* _S=(CSector*)(*_F);
                EVisible vis=_S->Intersect(pos,range);
                if (vis==fvPartialOutside)
                    sectors.push_back((u16)_S->m_sector_num);
            }
            if (sectors.empty()) return FALSE;
        }else{
            sectors.push_back((u16)m_iDefaultSectorNum);
        }
    }


    switch (e->m_Type){
    case ELight::ltPoint:		return BuildPointLight	(&L,e->m_Flags,lpSectors,e->m_FuzzyData?&e->m_FuzzyData->m_Positions:0,&e->_Transform());
    default:
    	THROW2("Invalid light type.");
	    return FALSE;
    }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Glow build functions
//------------------------------------------------------------------------------
BOOL SceneBuilder::BuildGlow(CGlow* e)
{
	l_glows.push_back(b_glow());
    b_glow& b 		= l_glows.back();
// material
    b_material mtl; ZeroMemory(&mtl,sizeof(mtl));
    int mtl_idx;
    VERIFY			(e->m_ShaderName.size());
	mtl.surfidx		= (u16)BuildTexture		(*e->m_TexName);		
    mtl.shader      = (u16)BuildShader		(*e->m_ShaderName);
    mtl.sector		= (u16)CalculateSector	(e->PPosition,e->m_fRadius);
    mtl.shader_xrlc	= -1;
    if ((u16(-1)==mtl.surfidx)||(u16(-1)==mtl.shader)) return FALSE;

    mtl_idx 		= FindInMaterials(&mtl);
    if (mtl_idx<0){
        l_materials.push_back(mtl);
        mtl_idx 	= l_materials.size()-1;
    }

// fill params
	b.P.set        	(e->PPosition);
    b.size        	= e->m_fRadius;
	b.dwMaterial   	= mtl_idx;
    b.flags			= e->m_Flags.is(CGlow::gfFixedSize)?0x01:0x00;	// 0x01 - non scalable
    return TRUE;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Portal build functions
//------------------------------------------------------------------------------
void SceneBuilder::BuildPortal(b_portal* b, CPortal* e){
	b->sector_front	= (u16)e->m_SectorFront->m_sector_num;
	b->sector_back	= (u16)e->m_SectorBack->m_sector_num;
    b->vertices.resize(e->m_SimplifyVertices.size());
    CopyMemory(b->vertices.begin(),e->m_SimplifyVertices.begin(),e->m_SimplifyVertices.size()*sizeof(Fvector));
}

//------------------------------------------------------------------------------
// shader build functions
//------------------------------------------------------------------------------
int SceneBuilder::FindInShaders(b_shader* s){
    for (u32 i=0; i<l_shaders.size(); i++)
        if(strcmp(l_shaders[i].name, s->name)==0)return i;
    return -1;
}
//------------------------------------------------------------------------------

int SceneBuilder::BuildShader(const char * s)
{
    b_shader sh;
    strcpy(sh.name,s);
    int sh_id = FindInShaders(&sh);
    if (sh_id<0){
        if (!EDevice.Resources->_FindBlender(sh.name)){
        	ELog.DlgMsg(mtError,"Can't find engine shader: %s",sh.name);
            return -1;
        }
        l_shaders.push_back(sh);
        return l_shaders.size()-1;
    }
    return sh_id;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// shader xrlc build functions
//------------------------------------------------------------------------------
int SceneBuilder::FindInShadersXRLC(b_shader* s){
    for (u32 i=0; i<l_shaders_xrlc.size(); i++)
        if(strcmp(l_shaders_xrlc[i].name, s->name)==0)return i;
    return -1;
}
//------------------------------------------------------------------------------

int SceneBuilder::BuildShaderXRLC(const char * s){
    b_shader sh;
    strcpy(sh.name,s);
    int sh_id = FindInShadersXRLC(&sh);
    if (sh_id<0){
        if (!EDevice.ShaderXRLC.Get(sh.name)){
        	ELog.DlgMsg(mtError,"Can't find compiler shader: %s",sh.name);
            return -1;
        }
        l_shaders_xrlc.push_back(sh);
        return l_shaders_xrlc.size()-1;
    }
    return sh_id;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// texture build functions
//------------------------------------------------------------------------------
int SceneBuilder::FindInTextures(const char* name){
    for (u32 i=0; i<l_textures.size(); i++)
    	if(stricmp(l_textures[i].name,name)==0) return i;
    return -1;
}
//------------------------------------------------------------------------------

int SceneBuilder::BuildTexture(const char* name)
{
	if (!(name&&name[0])){
		ELog.DlgMsg(mtError,"Invalid texture name found.");
        return -1;
    }
    int tex_idx     	= FindInTextures(name);
    if(tex_idx<0){
		b_texture       tex;
        ZeroMemory(&tex,sizeof(tex));
		strcpy          (tex.name,name);
    	l_textures.push_back(tex);
		tex_idx         = l_textures.size()-1;
    }
    return tex_idx;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// material build functions
//------------------------------------------------------------------------------
int SceneBuilder::FindInMaterials(b_material* m)
{
    for (u32 i=0; i<l_materials.size(); i++){
        if( (l_materials[i].surfidx		== m->surfidx) 		&&
            (l_materials[i].shader		== m->shader) 		&&
            (l_materials[i].shader_xrlc	== m->shader_xrlc) 	&&
            (l_materials[i].sector		== m->sector)) return i;
    }
    return -1;
}
//------------------------------------------------------------------------------

int SceneBuilder::BuildMaterial(CSurface* surf, int sector_num, bool allow_draft)
{
	return BuildMaterial(surf->_ShaderName(),surf->_ShaderXRLCName(),surf->_Texture(),((surf->_FVF()&D3DFVF_TEXCOUNT_MASK)>>D3DFVF_TEXCOUNT_SHIFT),sector_num,allow_draft);
}
int SceneBuilder::BuildMaterial(LPCSTR esh_name, LPCSTR csh_name, LPCSTR tx_name, u32 tx_cnt, int sector_num, bool allow_draft)
{
    b_material mtl; ZeroMemory(&mtl,sizeof(mtl));
    VERIFY(sector_num>=0);
    int mtl_idx;
	VERIFY			(tx_cnt==1);
    
    if (allow_draft&&(Scene->m_LevelOp.m_BuildParams.m_quality==ebqDraft)){
        Shader_xrLC* c_sh	= EDevice.ShaderXRLC.Get(csh_name);
        if (c_sh->flags.bRendering){
            mtl.shader      = (u16)BuildShader		("def_shaders\\def_vertex");
            mtl.shader_xrlc	= (u16)BuildShaderXRLC	("def_shaders\\def_vertex");
        }else{
            mtl.shader      = (u16)BuildShader		(esh_name);
            mtl.shader_xrlc	= (u16)BuildShaderXRLC	(csh_name);
        }
    }else{
	    mtl.shader      = (u16)BuildShader		(esh_name);
    	mtl.shader_xrlc	= (u16)BuildShaderXRLC	(csh_name);
    }
    mtl.sector		= (u16)sector_num;
	mtl.surfidx		= (u16)BuildTexture		(tx_name);
    if ((u16(-1)==mtl.shader)||(u16(-1)==mtl.shader_xrlc)||(u16(-1)==mtl.surfidx)) return -1;

    mtl_idx 		= FindInMaterials(&mtl);
    if (mtl_idx<0){
        l_materials.push_back(mtl);
        mtl_idx 	= l_materials.size()-1;
    }
    return mtl_idx;
}
//------------------------------------------------------------------------------

BOOL SceneBuilder::ParseStaticObjects(ObjectList& lst, LPCSTR prefix, bool b_selected_only)
{
	BOOL bResult = TRUE;
    SPBItem* pb	= UI->ProgressStart(lst.size(),"Parse static objects...");
    for(ObjectIt _F = lst.begin();_F!=lst.end();_F++)
	{
        pb->Inc((*_F)->Name);
        if (UI->NeedAbort()) break;
		if(b_selected_only && !(*_F)->Selected())
			continue;

        switch((*_F)->ClassID){
        case OBJCLASS_LIGHT:
            bResult = BuildLight((CLight*)(*_F));
            break;
        case OBJCLASS_GLOW:
            bResult = BuildGlow((CGlow*)(*_F));
            break;
        case OBJCLASS_PORTAL:
            l_portals.push_back(b_portal());
            BuildPortal(&l_portals.back(),(CPortal*)(*_F));
            break;
        case OBJCLASS_SCENEOBJECT:{
            CSceneObject *obj = (CSceneObject*)(*_F);
            if (obj->IsStatic()) 		
				bResult = BuildObject(obj);
            else if (obj->IsMUStatic())
				bResult = BuildMUObject(obj);
        }break;
/*        case OBJCLASS_GROUP:{
            CGroupObject* group = (CGroupObject*)(*_F);

            ObjectList 			grp_lst;
            group->GetObjects	(grp_lst);
            
            bResult = ParseStaticObjects(grp_lst, group->Name, b_selected_only);
        }break;   */
        }// end switch
        if (!bResult)
        {
            ELog.DlgMsg(mtError,"Failed to build object: '%s'",(*_F)->Name);
        	break;
        }
    }
    UI->ProgressEnd(pb);
    return bResult;
}
//------------------------------------------------------------------------------

BOOL SceneBuilder::CompileStatic(bool b_selected_only)
{
    ObjClassID cls = LTools->CurrentClassID();
    if(cls==OBJCLASS_DUMMY)	return FALSE;
	ESceneToolBase* pCurrentTool 	= Scene->GetOTool(cls);

	BOOL bResult	= TRUE;

    Clear			();

    SceneToolsMapPairIt t_it 	= Scene->FirstTool();
    SceneToolsMapPairIt t_end 	= Scene->LastTool();

    if(b_selected_only)
    {
    	if(pCurrentTool)
        	pCurrentTool->CompileStaticStart();
    }else
    {
        for (; t_it!=t_end; ++t_it)
        {
            ESceneToolBase* mt 		= t_it->second;
            if (mt)
                mt->CompileStaticStart();
        }
	}
	int objcount = Scene->ObjCount();
	if( objcount <= 0 )	return FALSE;

// compute vertex/face count
    l_vert_cnt	= 0;
    l_face_cnt 	= 0;
	l_vert_it 	= 0;
	l_face_it	= 0;


    if(b_selected_only)
    {
    	if(pCurrentTool)
           pCurrentTool->GetStaticDesc(l_vert_cnt,l_face_cnt, b_selected_only,false);
    }else
    {
        t_it 				= Scene->FirstTool();
        t_end 				= Scene->LastTool();
        for (; t_it!=t_end; ++t_it)
        {
            ESceneToolBase* mt 		= t_it->second;
            if (mt)
                mt->GetStaticDesc(l_vert_cnt,l_face_cnt, b_selected_only,false);
        }
    }
	l_faces		= xr_alloc<b_face>	(l_face_cnt);
    l_smgroups  = xr_alloc<u32>		(l_face_cnt);
	l_verts		= xr_alloc<b_vertex>(l_vert_cnt);

    l_scene_stat= xr_new<CSceneStat>(m_LevelBox);

// make hemisphere
	ESceneLightTool* lt = dynamic_cast<ESceneLightTool*>(Scene->GetOTool(OBJCLASS_LIGHT));
    LPCSTR h_control	= *lt->FindLightControl(lt->m_HemiControl)->name;
	BuildHemiLights		(Scene->m_LevelOp.m_LightHemiQuality,h_control);
    if (0!=strcmp(LCONTROL_HEMI,h_control))
		BuildHemiLights	(Scene->m_LevelOp.m_LightHemiQuality,LCONTROL_HEMI);
// make sun
	BuildSun			(Scene->m_LevelOp.m_LightSunQuality,lt->m_SunShadowDir);
// parse scene
    SPBItem* pb = UI->ProgressStart(Scene->ObjCount(),"Parse scene objects...");

    if(b_selected_only)
    {
    	if(pCurrentTool)
            if (!pCurrentTool->ExportStatic(this,b_selected_only))
                {bResult = FALSE;}
    }else
    {
        t_it 				= Scene->FirstTool();
        t_end 				= Scene->LastTool();
        for (; t_it!=t_end; ++t_it)
        {
            ESceneToolBase* mt = t_it->second;
            if (mt)
                if (!mt->ExportStatic(this,b_selected_only))
                    {bResult = FALSE; break;}
        }
    }
	UI->ProgressEnd(pb);
// process lods
	if (bResult&&!l_lods.empty())
    {
        SPBItem* pb = UI->ProgressStart(l_lods.size()*2,"Merge LOD textures...");
        Fvector2Vec			offsets;
        Fvector2Vec			scales;
        boolVec				rotated;
        U32Vec				remap;
        SSimpleImageVec		images;
        for (int k=0; k<(int)l_lods.size(); ++k)
        {
            images.push_back(SSimpleImage());
            SSimpleImage& I	= images.back();
            I.name			= l_lods[k].lod_name;
            I.layers.push_back(l_lods[k].data);
            I.layers.push_back(l_lods[k].ndata);
            I.w				= LOD_IMAGE_SIZE*LOD_SAMPLE_COUNT;
            I.h				= LOD_IMAGE_SIZE;
	        pb->Inc();
        }

        SSimpleImage merged_image;
        xr_string fn_color	= ChangeFileExt	(MakeLevelPath(LEVEL_LODS_TEX_NAME).c_str(),".dds").c_str();
        xr_string fn_normal	= ChangeFileExt	(MakeLevelPath(LEVEL_LODS_NRM_NAME).c_str(),".dds").c_str();
        if (1==ImageLib.CreateMergedTexture	(2,images,merged_image,512,2048,64,2048,offsets,scales,rotated,remap)){
            // all right, make texture
            STextureParams 		tp;
            tp.width			= merged_image.w;
            tp.height			= merged_image.h;
            tp.fmt				= STextureParams::tfDXT5;
            tp.type				= STextureParams::ttImage;
            tp.mip_filter		= STextureParams::kMIPFilterAdvanced;
            tp.flags.assign		(STextureParams::flDitherColor|STextureParams::flGenerateMipMaps);
            ImageLib.MakeGameTexture		(fn_color.c_str(),merged_image.layers[0].begin(), tp);
            ImageLib.MakeGameTexture		(fn_normal.c_str(),merged_image.layers[1].begin(),tp);
	        for (k=0; k<(int)l_lods.size(); k++){        
	            e_b_lod& l	= l_lods[k];         
                for (u32 f=0; f<8; f++){
                	for (u32 t=0; t<4; t++){
                    	Fvector2& uv = l.lod.faces[f].t[t];
                        u32 id 		 = remap[k];
                    	ImageLib.MergedTextureRemapUV(uv.x,uv.y,uv.x,uv.y,offsets[id],scales[id],rotated[id]);
                    }
                }
		        pb->Inc();
			}
        }else{
            ELog.DlgMsg		(mtError,"Failed to build merged LOD texture. Merged texture more than [2048x2048].");
        	bResult			= FALSE;
        }
        UI->ProgressEnd(pb);
    }

// save build    
    if (bResult && !UI->NeedAbort())
	{
		if(m_save_as_object)
			SaveBuildAsObject	();
		else
			SaveBuild			();
	}

    if(b_selected_only)
    {
    	if(pCurrentTool)
			pCurrentTool->CompileStaticEnd();
    }else
    {
        t_it 				= Scene->FirstTool();
        t_end 				= Scene->LastTool();
        for (; t_it!=t_end; ++t_it)
        {
            ESceneToolBase* mt 		= t_it->second;
            if (mt)
                mt->CompileStaticEnd();
        }
	}
    return bResult;
}

BOOL SceneBuilder::BuildSceneStat()
{
    xr_string dest_name = MakeLevelPath(LEVEL_DI_TEX_NAME);
    return l_scene_stat->flush(dest_name.c_str());
}
