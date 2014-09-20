#include "stdafx.h"
#pragma hdrstop

#include "ImageManager.h"
#include "Image.h"
#include "editobject.h"
#include "editmesh.h"
#include "ui_main.h"
#include "xrHemisphere.h"
#include "xrImage_Resampler.h"
#include "ETools.h"
/*
IC void SetCamera(float angle, const Fvector& C, float height, float radius, float dist)
{
    Fvector 	D;
    Fvector 	hpb;
    Fmatrix 	P;

	hpb.set		(angle,0,0);
	D.setHP		(hpb.x,hpb.y);
    D.mul		(-dist);
    D.add		(C);

    float ta	= height/dist;
    float asp 	= height/radius;
	float fp	= dist+4.f*radius;
    float np	= dist-4.f*radius; clamp(np,0.1f,fp);
    EDevice.m_Camera.Set		(hpb,D);
    P.build_projection_HAT	(ta,asp,np,fp);
    RCache.set_xform_project(P);
}

IC void CopyLODImage(U32Vec& src, U32Vec& dest, u32 src_w, u32 src_h, int id, int pitch)
{
	for (u32 y=0; y<src_h; y++)
    	CopyMemory(dest.begin()+y*pitch+id*src_w,src.begin()+y*src_w,src_w*sizeof(u32));
}

void CImageManager::CreateLODTexture(const Fbox& bb, U32Vec& tgt_data, u32 tgt_w, u32 tgt_h, int samples)
{
	U32Vec pixels;

    Fvector C;
    Fvector S;
    bb.getradius				(S);
    float R 					= _max(S.x,S.z);
    bb.getcenter				(C);

    Fmatrix save_projection		= EDevice.mProjection;
    Fvector save_pos 			= EDevice.m_Camera.GetPosition();
    Fvector save_hpb 			= EDevice.m_Camera.GetHPB();
    float save_far		 		= EDevice.m_Camera._Zfar();
	ECameraStyle save_style 	= EDevice.m_Camera.GetStyle();

    float D		= 500.f;
    u32 pitch 					= tgt_w*samples;

    tgt_data.resize				(pitch*tgt_h);
	EDevice.m_Camera.SetStyle	(csPlaneMove);
    EDevice.m_Camera.SetDepth	(D*2,true);

    // save render params
    Flags32 old_flag			= psDeviceFlags;
	u32 old_dwFillMode			= EDevice.dwFillMode;
    u32 old_dwShadeMode			= EDevice.dwShadeMode;
    // set render params

    u32 cc						= 	EPrefs->scene_clear_color;
    EPrefs.scene_clear_color 	= 	0x0000000;
    psDeviceFlags.zero			();
	psDeviceFlags.set			(rsFilterLinear,TRUE);
	EDevice.dwFillMode			= D3DFILL_SOLID;
    EDevice.dwShadeMode			= D3DSHADE_GOURAUD;

    SetCamera(0,C,S.y,R,D);

    for (int frame=0; frame<samples; frame++){
    	float angle 			= frame*(PI_MUL_2/samples);
	    SetCamera				(angle,C,S.y,R,D);
	    EDevice.MakeScreenshot	(pixels,tgt_w,tgt_h);
        // copy LOD to final
		for (u32 y=0; y<tgt_h; y++)
    		CopyMemory			(tgt_data.begin()+y*pitch+frame*tgt_w,pixels.begin()+y*tgt_w,tgt_w*sizeof(u32));
    }

    ApplyBorders				(tgt_data,pitch,tgt_h);

    // flip data
	for (u32 y=0; y<tgt_h/2; y++){
		u32 y2 = tgt_h-y-1;
		for (u32 x=0; x<pitch; x++){
        	std::swap	(tgt_data[y*pitch+x],tgt_data[y2*pitch+x]);	
		}
	}
        
    // restore render params
	EDevice.dwFillMode			= old_dwFillMode;
    EDevice.dwShadeMode			= old_dwShadeMode;
    psDeviceFlags 				= old_flag;
    EPrefs.scene_clear_color 	= cc;

	EDevice.m_Camera.SetStyle	(save_style);
    RCache.set_xform_project	(save_projection);
    EDevice.m_Camera.Set			(save_hpb,save_pos);
    EDevice.m_Camera.Set			(save_hpb,save_pos);
    EDevice.m_Camera.SetDepth	(save_far,false);
}
*/

DEFINE_VECTOR(Fvector4,Fvector4Vec,Fvector4It);
BOOL GetPointColor(SPickQuery::SResult* R, u32& alpha, u32& color)
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

/*    
//	float filter_core[3][3]={{0.0125,0.0125,0.0125},{0.0125,0.9,0.0125},{0.0125,0.0125,0.0125}};
//	float filter_core[3][3]={{0.0625,0.0625,0.0625},{0.0625,0.5,0.0625},{0.0625,0.0625,0.0625}};
//	float filter_core[3][3]={{0,0,0},{0,1,0},{0,0,0}};
    Fvector4 C={0,0,0,0};
    u32 cnt=0;
    for (int y=-1; y<=1; y++){
        int v 			= V+y;
        if (v<0||v>=(int)surf->m_ImageData->h) continue;
	    for (int x=-1; x<=1; x++){
	        int u 		= U+x;
	        if (u<0||u>=(int)surf->m_ImageData->w) continue;
        	Fvector4 	c;
            color		= surf->m_ImageData->layers.back()[v*surf->m_ImageData->w+u];
            float f 	= filter_core[x+1][y+1];
		    c.set		(f*color_get_R(color),f*color_get_G(color),f*color_get_B(color),f*color_get_A(color));
            C.add		(c);
            cnt++;
        }
    }
//	if (0!=cnt)	C.div(cnt);
    color	= color_rgba(C.x,C.y,C.z,C.w);
*/
	color = surf->m_ImageData->layers.back()[V*surf->m_ImageData->w+U];
    alpha = color_get_A(color);
    return TRUE;
}

struct SBuildLight{
    Flight					light;
    float					energy;
};
DEFINE_VECTOR				(SBuildLight,BLVec,BLIt);
ICF static void simple_hemi_callback(float x, float y, float z, float E, LPVOID P)
{
    BLVec* dst 					= (BLVec*)P;
    SBuildLight 				T;
    T.energy     				= E;
    T.light.direction.set		(x,y,z);
    dst->push_back  			(T);
}

const u32 lod_ss_quality  = 8;

void CreateLODSamples(const Fbox& bbox, U32Vec& tgt_data, u32 tgt_w, u32 tgt_h)
{
	U32Vec 	s_pixels,d_pixels;
    Fmatrix save_projection		= EDevice.mProject;
    Fmatrix save_view			= EDevice.mView;

    // save render params
    Flags32 old_flag			= psDeviceFlags;
	u32 old_dwFillMode			= EDevice.dwFillMode;
    u32 old_dwShadeMode			= EDevice.dwShadeMode;
    // set render params

    u32 cc						= 	EPrefs->scene_clear_color;
    EPrefs->scene_clear_color 	= 	0x0000000;
    psDeviceFlags.zero			();
	psDeviceFlags.set			(rsFilterLinear,TRUE);
	EDevice.dwFillMode			= D3DFILL_SOLID;
    EDevice.dwShadeMode			= D3DSHADE_GOURAUD;

    Fvector vP,vD,vN,vR;
    Fmatrix mV,mP;
    
    Fvector S;
    bbox.getradius				(S);
    float R 					= 2.f*_max(S.x,S.z);

    u32 pitch 					= tgt_w*LOD_SAMPLE_COUNT;
    tgt_data.resize				(pitch*tgt_h);
    for (int frame=0; frame<LOD_SAMPLE_COUNT; frame++){
        float angle 			= frame*(PI_MUL_2/LOD_SAMPLE_COUNT);

		Fbox bb 				= bbox;
        // build camera matrix
        bb.getcenter			( vP );
        vN.set					( 0.f,1.f,0.f );
        vD.setHP				( angle, 0 );
        vR.crossproduct			( vN,vD );
        mV.build_camera_dir		(vP,vD,vN);
        bb.xform				(mV);
        // build project matrix
        mP.build_projection_ortho(R,bb.max.y-bb.min.y,bb.min.z,bb.max.z);
	    RCache.set_xform_project(mP);
	    RCache.set_xform_view	(mV);
        EDevice.mFullTransform.mul(mP,mV);
        EDevice.MakeScreenshot	(s_pixels,tgt_w*lod_ss_quality,tgt_h*lod_ss_quality);
        d_pixels.resize 		(tgt_w*tgt_h);
		imf_Process				(d_pixels.begin(),tgt_w,tgt_h,s_pixels.begin(),tgt_w*lod_ss_quality,tgt_h*lod_ss_quality,imf_box);
        // copy LOD to final
		for (u32 y=0; y<tgt_h; y++)
    		CopyMemory			(tgt_data.begin()+y*pitch+frame*tgt_w,d_pixels.begin()+y*tgt_w,tgt_w*sizeof(u32));
	}

    // flip data
	for (u32 y=0; y<tgt_h/2; y++){
		u32 y2 = tgt_h-y-1;
		for (u32 x=0; x<pitch; x++){
        	std::swap	(tgt_data[y*pitch+x],tgt_data[y2*pitch+x]);	
		}
	}

    // restore render params
	EDevice.dwFillMode			= old_dwFillMode;
    EDevice.dwShadeMode			= old_dwShadeMode;
    psDeviceFlags 				= old_flag;
    EPrefs->scene_clear_color 	= cc;

    RCache.set_xform_view		(save_view);
    RCache.set_xform_project	(save_projection);
}

void CImageManager::CreateLODTexture(CEditableObject* OBJECT, U32Vec& lod_pixels, U32Vec& nm_pixels, u32 tgt_w, u32 tgt_h, int _samples, int quality)
{
	// build hemi light
    BLVec						simple_hemi;
    // fill simple hemi
    simple_hemi.clear			();
    xrHemisphereBuild			(1,2.f,simple_hemi_callback,&simple_hemi);

    Fbox bb						= OBJECT->GetBox();

    // build lod normals
    lod_pixels.resize			(LOD_IMAGE_SIZE*LOD_IMAGE_SIZE*LOD_SAMPLE_COUNT,0);
    nm_pixels.resize			(LOD_IMAGE_SIZE*LOD_IMAGE_SIZE*LOD_SAMPLE_COUNT,0);
    U32Vec hemi_tmp				(LOD_IMAGE_SIZE*LOD_IMAGE_SIZE*LOD_SAMPLE_COUNT,0); 
    Fvector 					o_center, o_size;
    Fmatrix 					M, Mi;
    bb.getradius				(o_size);
    bb.getcenter				(o_center);
    SPBItem* PB					= UI->ProgressStart(LOD_SAMPLE_COUNT*LOD_IMAGE_SIZE,OBJECT->GetName());
    float dW 					= _max(o_size.x,o_size.z)/(LOD_IMAGE_SIZE/2);
    float dH 					= o_size.y/(LOD_IMAGE_SIZE/2);
    float dR					= bb.getradius();
    float d2R					= dR*2.f;
	ETOOLS::ray_options			(CDB::OPT_CULL);

    CreateLODSamples			(bb,lod_pixels,LOD_IMAGE_SIZE,LOD_IMAGE_SIZE);

    float tN=0.f,tH=0.f,tT=0.f,tR=0.f;
    
	float 	LOD_CALC_SAMPLES 	= quality;
	s32		LOD_CALC_SAMPLES_LIM= LOD_CALC_SAMPLES/2;

    // preload textures
    for (SurfaceIt surf_it=OBJECT->Surfaces().begin(); surf_it!=OBJECT->Surfaces().end(); surf_it++){
    	CSurface* surf			= *surf_it;
        Shader_xrLC* c_sh		= EDevice.ShaderXRLC.Get(surf->_ShaderXRLCName());
        if (!c_sh->flags.bRendering) continue;
        if (0==surf->m_ImageData)surf->CreateImageData();
    }    

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
	        	float X			= (iW-(LOD_IMAGE_SIZE)/2)*dW;

                u32 pixel		= (LOD_IMAGE_SIZE-iH-1)*LOD_SAMPLE_COUNT*LOD_IMAGE_SIZE+LOD_IMAGE_SIZE*sample_idx+iW;
                u32& tgt_c		= lod_pixels[pixel];
                u32& tgt_n		= nm_pixels[pixel];
                u32& tgt_h		= hemi_tmp[pixel];

///.			tgt_c			= 0x00000000;
                
                FvectorVec		n_vec;
///.			Fvector4Vec		c_vec;
                Fvector4Vec		sample_pt_vec;
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
                        OBJECT->RayQuery		(PQ);
                        if (PQ.r_count()){
                            PQ.r_sort();
                            Fvector N	= {0,0,0};
///.		                Fcolor C	= {0,0,0,0};
                            for (s32 k=PQ.r_count()-1; k>=0; k--){
                            	SPickQuery::SResult* R = PQ.r_begin()+k;
								u32 	uA, uC;
                            	if (!GetPointColor(R,uA,uC)) continue;
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

                                // color
///.							Fcolor Cn;
///.							Cn.set			(uC);
///.							if (Cn.a>=(200.f/255.f)) C.lerp(C,Cn,Cn.a);
                            }
                            float n_mag			= N.magnitude();
                            if (!fis_zero(n_mag,EPS))
                                n_vec.push_back	(N.div(n_mag));
///.		                c_vec.push_back 	(Fvector4().set(C.r,C.g,C.b,C.a));
                        }
                    }
                }
tN+=TT.GetElapsed_sec();
///.			Fvector4 cC			= {0,0,0,0};
///.			for (Fvector4It c_it=c_vec.begin(); c_it!=c_vec.end(); c_it++)
///.				cC.add			(*c_it);
///.			cC.div				(c_vec.size());
///.			cC.mul				(255.f);
///.			tgt_c				= color_rgba(iFloor(cC.x),iFloor(cC.y),iFloor(cC.z),iFloor(cC.w)>200.f?255:0);

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
                tgt_n				= color_rgba(iFloor(N.x),iFloor(N.y),iFloor(N.z),color_get_A(tgt_c));

                if (0==color_get_A(tgt_c)) continue;
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
                        OBJECT->RayQuery		(PQ);
tR+=TT1.GetElapsed_sec();                             
                        float ray_transp= 1.f;
                        if (PQ.r_count()){
                            for (s32 k=0; k<PQ.r_count(); k++){
                                u32 	a,uC;
TT1.Start();
                                if (!GetPointColor(PQ.r_begin()+k,a,uC)) continue;
tT+=TT1.GetElapsed_sec();
                                ray_transp		*= (1.f - float(a)/255.f);
								if (fis_zero(ray_transp,EPS_L)) break;
                            }
                        }
                        avg_transp				+= ray_transp;
                    }
                    avg_transp					/= simple_hemi.size();
                    res_transp					= res_transp*(1.f-pt_it->w)+avg_transp*pt_it->w;
                }
tH+=TT.GetElapsed_sec();
				u8 h 				= (u8)iFloor	(res_transp*255.f);
				tgt_h				= color_rgba(h,h,h,color_get_A(tgt_c));
            }
        }
    }

	Msg("Normal: %3.2fsec, Hemi: %3.2f, PC: %3.2f, RP: %3.2f",tN,tH,tT,tR);
	ImageLib.ApplyBorders			(lod_pixels,LOD_IMAGE_SIZE*LOD_SAMPLE_COUNT,LOD_IMAGE_SIZE);
	ImageLib.ApplyBorders			(nm_pixels,	LOD_IMAGE_SIZE*LOD_SAMPLE_COUNT,LOD_IMAGE_SIZE);
	ImageLib.ApplyBorders			(hemi_tmp,	LOD_IMAGE_SIZE*LOD_SAMPLE_COUNT,LOD_IMAGE_SIZE);
    // fill alpha to N-channel (HEMI)
    for (int px_idx=0; px_idx<int(nm_pixels.size()); px_idx++)
        nm_pixels[px_idx]			= subst_alpha(nm_pixels[px_idx],color_get_R(hemi_tmp[px_idx]));
    
    UI->ProgressEnd(PB);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void CImageManager::CreateLODTexture(CEditableObject* OBJECT, LPCSTR tex_name, u32 tgt_w, u32 tgt_h, int samples, int age, int quality)
{
    U32Vec 						lod_pixels, nm_pixels;

	CreateLODTexture			(OBJECT,lod_pixels,nm_pixels,tgt_w,tgt_h,samples,quality);

    string_path					out_name,src_name;
    CImage* I 					= xr_new<CImage>();
    // save lod
    strcpy						(src_name,tex_name);

    strcpy						(src_name, EFS.ChangeFileExt(src_name,".thm").c_str());
    FS.file_delete				(_textures_,src_name);

    strcpy						(src_name, EFS.ChangeFileExt(src_name,".tga").c_str());
    FS.update_path				(out_name,_textures_,src_name);
    I->Create					(tgt_w*samples,tgt_h,lod_pixels.begin());
//	I->Vflip					();
    I->SaveTGA					(out_name);
    FS.set_file_age				(out_name,age);
    SynchronizeTexture			(src_name,age);

    // save normal map
    strconcat					(sizeof(src_name),src_name, tex_name, "_nm");
    strcpy						(src_name,EFS.ChangeFileExt(src_name,".thm").c_str());
    FS.file_delete				(_textures_,src_name);

    strcpy						(src_name, EFS.ChangeFileExt(src_name,".tga").c_str());
    FS.update_path				(out_name,_textures_,src_name);
    I->Create					(tgt_w*samples,tgt_h,nm_pixels.begin());
//	I->Vflip					();
    I->SaveTGA					(out_name);
    FS.set_file_age				(out_name,age);
    SynchronizeTexture			(src_name,age);
    
    xr_delete					(I);
}


