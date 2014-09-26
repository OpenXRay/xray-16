#include "stdafx.h"      
#pragma hdrstop          
                              
#include "Builder.h"
#include "../ECore/Editor/ImageManager.h"
#include "SceneObject.h"
#include "../ECore/Editor/EditObject.h"
#include "../ECore/Editor/ui_main.h"
#include "scene.h"

//------------------------------------------------------------------------------
#define LEVEL_LODS_TEX_NAME "level_lods"
#define LEVEL_LODS_NRM_NAME "level_lods_nm"
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// lod build functions
//------------------------------------------------------------------------------
DEFINE_VECTOR(Fvector4,Fvector4Vec,Fvector4It);
BOOL GetPointColor(SPickQuery::SResult* R, u32& alpha)
{
    CSurface* surf			= R->e_mesh->GetSurfaceByFaceID(R->tag); VERIFY(surf);
    Shader_xrLC* c_sh		= EDevice.ShaderXRLC.Get(surf->_ShaderXRLCName());
    if (!c_sh->flags.bRendering) return FALSE;
    const Fvector2*			cuv[3];
    R->e_mesh->GetFaceTC	(R->tag,cuv);

    // barycentric coords
    // note: W,U,V order
    Fvector B;
    B.set	(1.0f - R->u - R->v,R->u,R->v);

    // calc UV
    Fvector2	uv;
    uv.x = cuv[0]->x*B.x + cuv[1]->x*B.y + cuv[2]->x*B.z;
    uv.y = cuv[0]->y*B.x + cuv[1]->y*B.y + cuv[2]->y*B.z;

    int U = iFloor(uv.x*float(surf->m_ImageData->w) + .5f);
    int V = iFloor(uv.y*float(surf->m_ImageData->h)+ .5f);
    U %= surf->m_ImageData->w;	if (U<0) U+=surf->m_ImageData->w;
    V %= surf->m_ImageData->h;	if (V<0) V+=surf->m_ImageData->h;

    alpha = color_get_A(surf->m_ImageData->layers.back()[V*surf->m_ImageData->w+U]);
    return TRUE;
}

int	SceneBuilder::BuildObjectLOD(const Fmatrix& parent, CEditableObject* E, int sector_num)
{
	if (!E->m_objectFlags.is(CEditableObject::eoUsingLOD)) return -1;
    xr_string lod_name = E->GetLODTextureName();

    b_material 		mtl;
    mtl.surfidx		= (u16)BuildTexture		(LEVEL_LODS_TEX_NAME);
    mtl.shader      = (u16)BuildShader		(E->GetLODShaderName());
    mtl.sector		= (u16)sector_num;
    mtl.shader_xrlc	= -1;
    if ((u16(-1)==mtl.surfidx)||(u16(-1)==mtl.shader)) return -2;

    int mtl_idx		= FindInMaterials(&mtl);
    if (mtl_idx<0){
        l_materials.push_back(mtl);
        mtl_idx 	= l_materials.size()-1;
    }

    l_lods.push_back(e_b_lod());
    e_b_lod& b		= l_lods.back();
    Fvector    		p[4];
    Fvector2 		t[4];
    for (int frame=0; frame<LOD_SAMPLE_COUNT; frame++){
        E->GetLODFrame(frame,p,t,&parent);
        for (int k=0; k<4; k++){
            b.lod.faces[frame].v[k].set(p[k]);
            b.lod.faces[frame].t[k].set(t[k]);
        }
    }
    b.lod.dwMaterial= mtl_idx;
    b.lod_name		= lod_name.c_str();
    xr_string l_name= lod_name.c_str();
    u32 w,h;     
    int age;
    if (!ImageLib.LoadTextureData(l_name.c_str(),b.data,w,h,&age)){
    	Msg("!Can't find LOD texture: '%s'",l_name.c_str());
    	return -2;
    }
/*    if (age!=E->Version()){
    	Msg("!Invalid LOD texture version: '%s'",l_name.c_str());
    	return -2;
    }*/
    l_name 			+= "_nm";
    if (!ImageLib.LoadTextureData(l_name.c_str(),b.ndata,w,h,&age)){
    	Msg("!Can't find LOD texture: '%s'",l_name.c_str());
    	return -2;
    }
/*    if (age!=E->Version()){
    	Msg("!Invalid LOD texture version: '%s'",l_name.c_str());
    	return -2;
    }*/
//    b.data
/*
    // make lod
    Fbox bb						= E->GetBox();
    E->m_Flags.set				(CEditableObject::eoUsingLOD,FALSE);
    object_for_render			= E;
    ImageLib.CreateLODTexture	(bb, b.data, LOD_IMAGE_SIZE,LOD_IMAGE_SIZE,LOD_SAMPLE_COUNT);
    E->m_Flags.set				(CEditableObject::eoUsingLOD,TRUE);

    // build lod normals
    b.ndata.resize				(LOD_IMAGE_SIZE*LOD_IMAGE_SIZE*LOD_SAMPLE_COUNT);
    U8Vec hemi_temp				(LOD_IMAGE_SIZE*LOD_IMAGE_SIZE*LOD_SAMPLE_COUNT,0);
    Fvector o_center,o_size;
    Fmatrix M,Mi;
    bb.getradius				(o_size);
    bb.getcenter				(o_center);
    SPBItem* PB					= UI->ProgressStart(LOD_SAMPLE_COUNT*LOD_IMAGE_SIZE,lod_name.c_str());
    float dW 					= _max(o_size.x,o_size.z)/(LOD_IMAGE_SIZE/2);
    float dH 					= o_size.y/(LOD_IMAGE_SIZE/2);
    float dR					= bb.getradius();
    float d2R					= dR*2.f;
	XRC.ray_options				(CDB::OPT_CULL);
	ETOOLS::ray_options			(CDB::OPT_CULL);

    float tN=0.f,tH=0.f,tT=0.f,tR=0.f;
    
	float 	LOD_CALC_SAMPLES 	= Scene->m_LevelOp.m_LOD_Quality;
	s32		LOD_CALC_SAMPLES_LIM= LOD_CALC_SAMPLES/2;

    // preload textures
    for (SurfaceIt surf_it=E->Surfaces().begin(); surf_it!=E->Surfaces().end(); surf_it++){
    	CSurface* surf			= *surf_it;
        Shader_xrLC* c_sh		= EDevice.ShaderXRLC.Get(surf->_ShaderXRLCName());
        if (!c_sh->flags.bRendering) continue;
        if (0==surf->m_ImageData)surf->CreateImageData();
    }    

    // preload CF Model
    for (EditMeshIt mesh_it=E->Meshes().begin(); mesh_it!=E->Meshes().end(); mesh_it++)
		(*mesh_it)->GenerateCFModel();
    
    // calculate
    for (u32 sample_idx=0; sample_idx<LOD_SAMPLE_COUNT; sample_idx++){
    	float angle 			= sample_idx*(PI_MUL_2/LOD_SAMPLE_COUNT);
        M.setXYZ				(0,angle,0);
        M.translate_over		(o_center);
        Mi.invert				(M);
	    for (s32 iH=0; iH<LOD_IMAGE_SIZE; iH++){
        	PB->Inc				();
        	float Y				= (iH-(LOD_IMAGE_SIZE-1)/2)*dH;
		    for (s32 iW=0; iW<LOD_IMAGE_SIZE; iW++){
	        	float X			= (iW-LOD_IMAGE_SIZE/2)*dW;

                u32 pixel		= (LOD_IMAGE_SIZE-iH-1)*LOD_SAMPLE_COUNT*LOD_IMAGE_SIZE+LOD_IMAGE_SIZE*sample_idx+iW;
                u32& tgt_n		= b.ndata[pixel];
                u8& tgt_h		= hemi_temp[pixel];
                u8 src_a		= color_get_A	(b.data[pixel]);

				if (0==src_a)	continue;
                
                FvectorVec		n_vec;
                Fvector4Vec		sample_pt_vec;//(iFloor(LOD_CALC_SAMPLES*LOD_CALC_SAMPLES));
//.				FvectorVec		c_vec;
				Fvector			start;
CTimer 	TT,TT1;
TT.Start();
                SPickQuery 		PQ;
                for (s32 iiH=-LOD_CALC_SAMPLES_LIM; iiH<=LOD_CALC_SAMPLES_LIM; iiH++){
                	float dY	= iiH*(dH/LOD_CALC_SAMPLES);
                    for (s32 iiW=-LOD_CALC_SAMPLES_LIM; iiW<=LOD_CALC_SAMPLES_LIM; iiW++){
	                	float dX= iiW*(dW/LOD_CALC_SAMPLES);
                        start.set		(X+dX,Y+dY,0);
                        M.transform_tiny(start);
                        start.mad		(M.k,-dR);
                        PQ.prepare_rq	(start,M.k,d2R,CDB::OPT_CULL);
                        E->RayQuery		(PQ);
                        if (PQ.r_count()){
                            PQ.r_sort();
                            Fvector N	= {0,0,0};
                            for (s32 k=PQ.r_count()-1; k>=0; k--){
                            	SPickQuery::SResult* R = PQ.r_begin()+k;
								u32 	uA;
                            	if (!GetPointColor(R,uA)) continue;
                                float	fA = float(uA)/255.f;
								if (uA){
                                    Fvector  pt; 	pt.mad(PQ.m_Start,PQ.m_Direction,R->range-EPS_L);
                                    Fvector4 ptt;	ptt.set(pt.x,pt.y,pt.z,fA);
                                    sample_pt_vec.push_back(ptt);
                                }
                                // normal
                                Fvector Nn;
                                Nn.mknormal		(R->verts[0],R->verts[1],R->verts[2]);
                                Nn.mul			(fA);

                                N.mul			(1.f-fA);
                                N.add			(Nn);
                            }
                            float n_mag			= N.magnitude();
                            if (!fis_zero(n_mag,EPS))
                                n_vec.push_back	(N.div(n_mag));
                        }
                    }
                }
tN+=TT.Stop();

TT.Start();
                // light points
                float res_transp		= 0.f;
                for (Fvector4It pt_it=sample_pt_vec.begin(); pt_it!=sample_pt_vec.end(); pt_it++){
                    float avg_transp	= 0.f;
                    for (BLIt it=simple_hemi.begin(); it!=simple_hemi.end(); it++){
TT1.Start();
                        Fvector 		start;
                        start.mad		(Fvector().set(pt_it->x,pt_it->y,pt_it->z),it->light.direction,-dR);
                        PQ.prepare_rq	(start,it->light.direction,dR,CDB::OPT_CULL);
                        E->RayQuery		(PQ);
tR+=TT1.Stop();                             
                        float ray_transp= 1.f;
                        if (PQ.r_count()){
                            for (s32 k=0; k<PQ.r_count(); k++){
                                u32 	a;
TT1.Start();
                                if (!GetPointColor(PQ.r_begin()+k,a)) continue;
tT+=TT1.Stop();
                                ray_transp		*= (1.f - float(a)/255.f);
								if (fis_zero(ray_transp,EPS_L)) break;
                            }
                        }
                        avg_transp				+= ray_transp;
                    }
                    avg_transp					/= simple_hemi.size();
                    res_transp					= res_transp*(1.f-pt_it->w)+avg_transp*pt_it->w;
                }
tH+=TT.Stop();

				tgt_h				= iFloor	(res_transp*255.f);

                Fvector N={0,0,0};
                if (!n_vec.empty()){
                    for (FvectorIt it=n_vec.begin(); it!=n_vec.end(); it++) N.add(*it);
                    N.div			(n_vec.size());
	                N.normalize_safe();
	                Mi.transform_dir(N);
                }
                N.mul				(0.5f);
                N.add				(0.5f);
                N.mul				(255.f);
                tgt_n				= color_rgba(iFloor(N.x),iFloor(N.y),iFloor(N.z),src_a);
            }
        }
    }

	Msg("Normal: %3.2fsec, Hemi: %3.2f, PC: %3.2f, RP: %3.2f",tN,tH,tT,tR);
	ImageLib.ApplyBorders			(b.ndata,	LOD_IMAGE_SIZE*LOD_SAMPLE_COUNT,LOD_IMAGE_SIZE);
    // fill alpha to N-channel
    for (int px_idx=0; px_idx<int(b.ndata.size()); px_idx++)
        b.ndata[px_idx]				= subst_alpha(b.ndata[px_idx],hemi_temp[px_idx]);
    UI->ProgressEnd(PB);
*/    
    
    return l_lods.size()-1;
}
//------------------------------------------------------------------------------

