#include "stdafx.h"
#pragma hdrstop

#include "GeometryPartExtractor.h"
#include "../ECore/Editor/EditObject.h"
#include "../ECore/Editor/UI_Main.h"
#include "UI_LevelTools.h"

//------------------------------------------------------------------------------
// Parts
//------------------------------------------------------------------------------
void SBPart::append_face			(SBFace* F)
{
    m_Faces.push_back				(F);
}
void SBPart::use_face				(SBFace* F, u32& cnt, u32 bone_id, float& area)
{
    VERIFY							(F->bone_id==-1);
    F->marked						= true;
    F->bone_id						= bone_id;
    area							+= F->CalcArea();
    cnt++;
}
void SBPart::recurse_fragment		(SBFace* F, u32& cnt, u32 bone_id, u32 max_faces, float& area)
{
    if (F){
        if (!F->marked)	use_face	(F,cnt,bone_id,area);
        // fill nearest
        SBFaceVec r_vec;
        for (SBFaceVecIt n_it=F->adjs.begin(); n_it!=F->adjs.end(); n_it++){
            if (cnt>=max_faces)		break;
            if ((*n_it)->marked)	continue;
            use_face				(*n_it,cnt,bone_id,area);
            r_vec.push_back			(*n_it);
        }     
        // recurse adjs   	
        for (SBFaceVecIt a_it=r_vec.begin(); a_it!=r_vec.end(); a_it++){
            if (cnt>=max_faces)					break;
            if ((*a_it)->bone_id!=(int)bone_id)	continue;
            recurse_fragment					(*a_it,cnt,bone_id,max_faces,area);
        } 
    }
}
bool SBPart::prepare				(SBAdjVec& adjs, u32 bone_face_min)
{
    m_bValid	= true;
    // compute OBB
    FvectorVec pts; pts.reserve		(m_Faces.size()*3);
    for (SBFaceVecIt f_it=m_Faces.begin(); f_it!=m_Faces.end(); f_it++){
        (*f_it)->marked				= false;
        for (int k=0; k<3; k++)		pts.push_back((*f_it)->o[k]);
    }
    ComputeOBB_RAPID				(m_OBB,pts,pts.size()/3);
    // fill adjacent
    for (SBFaceVecIt a_it=m_Faces.begin(); a_it!=m_Faces.end(); a_it++){
        SBFace* A					= *a_it;
        for (int k=0; k<3; k++){
            SBFaceVec& b_vec		= adjs[A->vert_id[k]];
            for (SBFaceVecIt b_it=b_vec.begin(); b_it!=b_vec.end(); b_it++){
                SBFace* B			= *b_it;
                if (A!=B){
                    int cnt			= 0;
                    for (int a=0; a<3; a++) for (int b=0; b<3; b++) if (A->vert_id[a]==B->vert_id[b]) cnt++;
                    if (cnt>=2){
                        if (std::find(A->adjs.begin(),A->adjs.end(),B)==A->adjs.end()) A->adjs.push_back(B);
                        if (std::find(B->adjs.begin(),B->adjs.end(),A)==B->adjs.end()) B->adjs.push_back(A);
                    }
                }
            }        
        }
    }
    // prepare transform matrix
    m_BBox.invalidate				();
    Fmatrix M; M.set				(m_OBB.m_rotate.i,m_OBB.m_rotate.j,m_OBB.m_rotate.k,m_OBB.m_translate);
    m_RefOffset.set					(m_OBB.m_translate);
    M.getXYZ						(m_RefRotate); // не i потому что в движке так
    M.invert						();

    // transform vertices & calculate bounding box
    for (f_it=m_Faces.begin(); f_it!=m_Faces.end(); f_it++){
        SBFace* F					= (*f_it);
        if (F->adjs.empty()){	
            ELog.Msg(mtError,"Error face found at pos: [%3.2f,%3.2f,%3.2f]",VPUSH(F->o[0])); 
            Tools->m_DebugDraw.AppendWireFace(F->o[0],F->o[1],F->o[2]);
            m_bValid				= false;
        }
        for (int k=0; k<3; k++){ 
            M.transform_dir			(F->n[k]);
            M.transform_tiny		(F->o[k]);
            m_BBox.modify			(F->o[k]);
        }
    }   
    if (m_bValid){
        // calculate bone params
        int bone_cnt_calc				= iFloor(float(m_Faces.size())/bone_face_min);
        int bone_cnt_max				= (bone_cnt_calc<62)?(bone_cnt_calc<=0?1:bone_cnt_calc):62;
        int bone_face_max				= iFloor(float(m_Faces.size())/bone_cnt_max+0.5f); bone_face_max *= 4.f;
        int bone_idx					= 0;
        // create big fragment
        u32 face_accum_total			= 0;
        AnsiString parent_bone			= "";
        do{
            SBFace* F					= 0;
            // find unused face
            for (SBFaceVecIt f_it=m_Faces.begin(); f_it!=m_Faces.end(); f_it++){
                if (!(*f_it)->marked){
                    F					= *f_it;
                    int cnt 			= 0;
                    for (SBFaceVecIt a_it=F->adjs.begin(); a_it!=F->adjs.end(); a_it++) cnt+=(*a_it)->marked?0:1;
                    if ((cnt==0)||(cnt>=2))	break;
                }
            }
            if (!F)						break;
            float area					= 0;
            u32 face_accum				= 0;
            u32 face_max_count 			= Random.randI(bone_face_min,bone_face_max+1);
            // fill faces
            recurse_fragment			(F,face_accum,bone_idx,face_max_count,area);
            if (face_accum==1){
//            	F->marked				= false;
                F->bone_id				= -1;
            }else{
                m_Bones.push_back		(SBBone(bone_idx,parent_bone,F->surf->_GameMtlName(),face_accum,area));
                parent_bone				= "0";
                bone_idx				++;
                face_accum_total		+= face_accum;
            }
            // create bone
        }while(bone_idx<bone_cnt_max);
        
        // attach small single face to big fragment
        u32 face_accum_total_saved 		= face_accum_total;
        while (face_accum_total<m_Faces.size()){
        	if (face_accum_total_saved!=face_accum_total){
				face_accum_total_saved 	= face_accum_total;
            }else{
            	// если проход оказалс€ безрезультатным (т.е. не смогли добавить хоть один фейс) - добавл€ем 
                //  все неприаттаченные фейсы к 0-кости (иначе зацикливаетс€ (mike - L03_agroprom))
                for (SBFaceVecIt f_it=m_Faces.begin(); f_it!=m_Faces.end(); f_it++){
                    SBFace* F				= *f_it;
                    if (-1==F->bone_id){
                        F->marked		= true;
                        F->bone_id		= 0;
                        face_accum_total++;
                    }
                } 
            }
            for (SBFaceVecIt f_it=m_Faces.begin(); f_it!=m_Faces.end(); f_it++){
                SBFace* F				= *f_it;
                if (-1==F->bone_id){
                    SBFace* P			= 0;
                    if (F->adjs.empty()){
                        F->marked		= true;
                        F->bone_id		= 0;
                        face_accum_total++;
                    }else{
                        for (SBFaceVecIt a_it=F->adjs.begin(); a_it!=F->adjs.end(); a_it++){ 
                            if (-1!=(*a_it)->bone_id){	
	                            P		= *a_it;
                            	break;
                            }
                        }
                    }
                    if (P){
                    	VERIFY(-1!=P->bone_id);
                        F->marked		= true;
                        F->bone_id		= P->bone_id;
                        face_accum_total++;
                    }
                }
            } 
        }
        
        // calculate bone offset
        for (f_it=m_Faces.begin(); f_it!=m_Faces.end(); f_it++){
            SBFace* F					= *f_it;
        	VERIFY						(F->bone_id!=-1);
            SBBone& B					= m_Bones[F->bone_id];
            for (int k=0; k<3; k++)		B.offset.add(F->o[k]);
        }
        for (SBBoneVecIt b_it=m_Bones.begin(); b_it!=m_Bones.end(); b_it++){
            SBBone& B					= *b_it;
            VERIFY						(0!=B.f_cnt);
            B.offset.div				(B.f_cnt*3);
        }
        Fvector& offs					= m_Bones.front().offset;
        for (b_it=m_Bones.begin(); b_it!=m_Bones.end(); b_it++)
            b_it->offset.sub			(offs);
    }
    return m_bValid;
}
bool SBPart::Export	(IWriter& F, u8 infl)
{
	VERIFY			(!m_Bones.empty());
    if (m_Bones.size()>63){
    	ELog.Msg(mtError,"Breakable object cannot handle more than 63 parts.");
     	return false;
    }

    bool bRes = true;

    u32 bone_count			= m_Bones.size();
                    
    xr_vector<FvectorVec>	bone_points;
	bone_points.resize		(bone_count);

    u32 mtl_cnt				= 0;
                                  
    for (SBFaceVecIt pf_it=m_Faces.begin(); pf_it!=m_Faces.end(); pf_it++){
    	SBFace* face		= *pf_it;
        int mtl_idx			= FindSplit(face->surf->_ShaderName(),face->surf->_Texture(),0);
        if (mtl_idx<0){
            m_Splits.push_back(SSplit(face->surf,m_BBox,0));
            mtl_idx	= mtl_cnt++;
        }
        SSplit& split=m_Splits[mtl_idx];
        SSkelVert v[3];
        for (int k=0; k<3; k++){
            st_SVert::bone b[1];
            b[0].id		= face->bone_id;
            b[0].w		= 1.f;
            v[k].set	(face->o[k],face->n[k],face->uv[k],1,b);
        }
        split.add_face		(v[0], v[1], v[2]);

        if (face->surf->m_Flags.is(CSurface::sf2Sided)){
            v[0].norm.invert(); v[1].norm.invert(); v[2].norm.invert();
            if (!split.add_face(v[0], v[2], v[1])) split.invalid_faces++;
        }
    }

    // fill per bone vertices
    for (SplitIt split_it=m_Splits.begin(); split_it!=m_Splits.end(); split_it++){
        if (!split_it->valid()){
            ELog.Msg(mtError,"Degenerate part found (Texture '%s').",*split_it->m_Texture);
            bRes = false;
            break;
        }
        if (0!=split_it->invalid_faces){
	        ELog.Msg(mtError,"Part [texture '%s'] have %d duplicate(degenerate) face(s).",*split_it->m_Texture,split_it->invalid_faces);
        }
    	// calculate T&B components
		split_it->CalculateTB();
        // subtract offset
		SkelVertVec& lst = split_it->getV_Verts();
	    for (SkelVertIt sv_it=lst.begin(); sv_it!=lst.end(); sv_it++){
		    bone_points[sv_it->bones[0].id].push_back(sv_it->offs);
            bone_points[sv_it->bones[0].id].back().sub(m_Bones[sv_it->bones[0].id].offset);
        }
    }

    if (!bRes) return false;

    // compute bounding
    ComputeBounding	();

	// create OGF
    // Header
    ogf_header 		H;
    H.format_version= xrOGF_FormatVersion;
    H.type			= MT_SKELETON_RIGID;
    H.shader_id		= 0;
    H.bb.min		= m_Box.min;
    H.bb.max		= m_Box.max;
    m_Box.getsphere	(H.bs.c,H.bs.r);
    F.w_chunk		(OGF_HEADER,&H,sizeof(H));

    // Desc
    ogf_desc		desc;
    F.open_chunk	(OGF_S_DESC);
    desc.Save		(F);
    F.close_chunk	();
	
    // OGF_CHILDREN
    F.open_chunk	(OGF_CHILDREN);
    int chield=0;
    for (split_it=m_Splits.begin(); split_it!=m_Splits.end(); split_it++){
	    F.open_chunk(chield++);
        split_it->Save(F);
	    F.close_chunk();
    }
    F.close_chunk();

    // BoneNames                   
    F.open_chunk(OGF_S_BONE_NAMES); 
    F.w_u32		(bone_count);
    // write other bones
    for (u32 bone_idx=0; bone_idx<bone_count; bone_idx++){
    	SBBone& bone=m_Bones[bone_idx];
        F.w_stringZ	(bone.name.c_str());
        F.w_stringZ	(bone.parent.c_str());
        Fobb		obb;
        ComputeOBB_WML	(obb,bone_points[bone_idx]);
        F.w				(&obb,sizeof(Fobb));
    }
    F.close_chunk();

    F.open_chunk(OGF_S_IKDATA);
    for (bone_idx=0; bone_idx<bone_count; bone_idx++){
    	SBBone& bone=m_Bones[bone_idx];

        F.w_u32		(0x0001); VERIFY(0x0001==OGF_IKDATA_VERSION);
        // material
        F.w_stringZ (bone.mtl.c_str());
        // shape
        SBoneShape	shape;
        shape.type	= SBoneShape::stBox;
        shape.flags.assign(SBoneShape::sfRemoveAfterBreak);
        ComputeOBB_WML	(shape.box,bone_points[bone_idx]);
	    F.w				(&shape,sizeof(SBoneShape));
        // IK data
        SJointIKData 	ik_data;
        ik_data.Reset	();
        ik_data.type	= jtNone;
        ik_data.ik_flags.set(SJointIKData::flBreakable,TRUE);
        ik_data.Export	(F);

        Fvector rot={0,0,0};
        F.w_fvector3(rot);
        F.w_fvector3(bone.offset);
        F.w_float   (bone.area);	// mass (дл€  ости посчитал площадь)
        F.w_fvector3(shape.box.m_translate);	// center of mass        
    }
    F.close_chunk();

    return bRes;
}

//------------------------------------------------------------------------------
// Extractor
//------------------------------------------------------------------------------
IC void recurse_tri(SBPart* P, SBFaceVec& faces, SBAdjVec& adjs, SBFace* F)
{
	if (F->marked) 		return;

	P->append_face		(F);
    F->marked			= true;

    // recurse
    for (int k=0; k<3; k++){
	    SBFaceVec& PL 	= adjs[F->vert_id[k]];
        for (SBFaceVecIt pl_it=PL.begin(); pl_it!=PL.end(); pl_it++)
            recurse_tri	(P,faces,adjs,*pl_it);
    }
}
//----------------------------------------------------
void CGeomPartExtractor::AppendFace(CSurface* surf, const Fvector* v, const Fvector* n, const Fvector2* uvs[3])
{
	SBFace* F			= xr_new<SBFace>(surf,uvs);
    // insert verts
    for (int k=0; k<3; k++){
        F->vert_id[k] 	= m_Verts->add_vert(v[k]);
	    F->o[k].set		(v[k]);
	    F->n[k].set		(n[k]);
    }
    m_Faces.push_back	(F);
}
CGeomPartExtractor::CGeomPartExtractor	()
{
	m_Verts				= 0;
}
void CGeomPartExtractor::Initialize	(const Fbox& bb, float eps, u32 per_bone_face_count_min)
{
	VERIFY				(0==m_Verts);
	m_Verts 			= xr_new<VCPacked>(bb,eps);
    m_PerBoneFaceCountMin=per_bone_face_count_min; VERIFY(m_PerBoneFaceCountMin>0);
}
void CGeomPartExtractor::Clear		()
{
    xr_delete		(m_Verts);
    for (SBFaceVecIt f_it=m_Faces.begin(); f_it!=m_Faces.end(); f_it++) 
    	xr_delete	(*f_it);
    m_Faces.clear	();
    for (SBPartVecIt p_it=m_Parts.begin(); p_it!=m_Parts.end(); p_it++) 
    	xr_delete	(*p_it);
    m_Parts.clear	();
    m_Adjs.clear	();
}
BOOL CGeomPartExtractor::Process()
{
    // make adjacement
    {
        m_Adjs.resize	(m_Verts->getVS());
        for (SBFaceVecIt f_it=m_Faces.begin(); f_it!=m_Faces.end(); f_it++)
            for (int k=0; k<3; k++) m_Adjs[(*f_it)->vert_id[k]].push_back(*f_it);
    }
    // extract parts
    {
        SPBItem* pb = UI->ProgressStart(m_Faces.size(),"Extract Parts...");
        for (SBFaceVecIt f_it=m_Faces.begin(); f_it!=m_Faces.end(); f_it++){
	        pb->Inc();
            SBFace* F	= *f_it;
            if (!F->marked){
                SBPart* P 		= xr_new<SBPart>();
                recurse_tri		(P,m_Faces,m_Adjs,*f_it);
                m_Parts.push_back	(P);
            }
        }
        UI->ProgressEnd(pb);
    }
    // simplify parts
    {
	    SPBItem* pb = UI->ProgressStart(m_Parts.size(),"Simplify Parts...");
        for (SBPartVecIt p_it=m_Parts.begin(); p_it!=m_Parts.end(); p_it++){	
	        pb->Inc();
        	(*p_it)->prepare	(m_Adjs,m_PerBoneFaceCountMin);
        }
	    UI->ProgressEnd(pb);
    }
    return TRUE;
}

