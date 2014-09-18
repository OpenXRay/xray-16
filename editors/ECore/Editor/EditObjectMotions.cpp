//----------------------------------------------------
// file: CEditableObject.cpp
//----------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "EditObject.h"

#ifdef _EDITOR
	#include "ui_main.h"
#endif

#include "motion.h"
#include "bone.h"
#include "EditMesh.h"


#ifdef _EDITOR
	#include "SkeletonAnimated.h"
	#include "AnimationKeyCalculate.h"
#endif

#ifndef _EDITOR
	bool check_scale( Fmatrix F ){ return true;}
#endif

//----------------------------------------------------
class fBoneNameEQ {
	shared_str	name;
public:
	fBoneNameEQ(shared_str N) : name(N) {};
	IC bool operator() (CBone* B) { return (xr_strcmp(B->Name(),name)==0); }
};
class fBoneWMNameEQ {
	shared_str	wm_name;
public:
	fBoneWMNameEQ(shared_str N) : wm_name(N) {};
	IC bool operator() (CBone* B) { return (xr_strcmp(B->WMap(),wm_name)==0); }
};
//----------------------------------------------------
#ifdef _EDITOR
extern CBone* 	bone_to_delete;
extern u32 		bone_to_delete_frame;

void CEditableObject::OnFrame()
{
	if (IsSkeleton()){
		BoneVec& lst = m_Bones;
    	if (IsSMotionActive()){
            Fvector R,T;
            int i=0;
            for(BoneIt b_it=lst.begin(); b_it!=lst.end(); b_it++, i++){
	            m_ActiveSMotion->_Evaluate(i,m_SMParam.Frame(),T,R);
                (*b_it)->_Update(T,R);
            }
            m_SMParam.Update(EDevice.fTimeDelta,m_ActiveSMotion->fSpeed,!m_ActiveSMotion->m_Flags.is(esmStopAtEnd));
        }else{
		    //for (BoneIt b_it=lst.begin(); b_it!=lst.end(); b_it++) (*b_it)->Reset();
        }
        CalculateAnimation(m_ActiveSMotion);
    }
    if(bone_to_delete)
    {
        if(EDevice.dwFrame > bone_to_delete_frame+3)
    		xr_delete(bone_to_delete);
    }
}
#endif

void CEditableObject::OnBindTransformChange()
{
    for(EditMeshIt mesh_it=FirstMesh();mesh_it!=LastMesh();mesh_it++){
        CEditableMesh* MESH = *mesh_it;
        MESH->UnloadSVertices(true);
    }
    GotoBindPose();
}

void CEditableObject::GotoBindPose()
{
    BoneVec& lst = m_Bones;
    for (BoneIt b_it=lst.begin(); b_it!=lst.end(); b_it++) (*b_it)->Reset();
    CalculateAnimation(0);
#ifdef _EDITOR
    UI->RedrawScene();
#endif
}

CSMotion* CEditableObject::ResetSAnimation(bool bGotoBindPose)
{
	CSMotion* M=m_ActiveSMotion;
	SetActiveSMotion(0);
    if (bGotoBindPose)
     		GotoBindPose();
    return M;
}

//----------------------------------------------------
// Skeletal motion
//----------------------------------------------------
static void CalculateAnimBone(CBone* bone, CSMotion* motion, Fmatrix& parent)
{
        Flags8 flags; flags.zero();
    if (motion)
     flags 	= motion->GetMotionFlags(bone->SelfID);

    Fmatrix& M 		= bone->_MTransform();
    Fmatrix& L 		= bone->_LTransform();
    
    const Fvector& r = bone->_Rotate();
    if( ! bone->callback_overwrite()  )
    {
        if ( flags.is(st_BoneMotion::flWorldOrient)){
            M.setXYZi	(r.x,r.y,r.z);
            M.c.set		(bone->_Offset());
            L.mul		(parent,M);
            L.i.set		(M.i);
            L.j.set		(M.j);
            L.k.set		(M.k);

            Fmatrix		 LI; LI.invert(parent);
            M.mulA_43	(LI);
        }else{
            M.setXYZi	(r.x,r.y,r.z);
            M.c.set		(bone->_Offset());
            L.mul		(parent,M);
        }
     }
    if( bone->callback() )
    {
    	bone->callback()( bone );
        M.mul_43( Fmatrix().invert(parent), L );
       // bone->_Offset().set( M.c );
    }

	bone->_RenderTransform().mul_43(bone->_LTransform(),bone->_RITransform());
}
static void CalculateAnim(CBone* bone, CSMotion* motion, Fmatrix& parent)
{

	CalculateAnimBone( bone, motion, parent );
    // Calculate children
    for (BoneIt b_it=bone->children.begin(); b_it!=bone->children.end(); b_it++)
    	CalculateAnim	(*b_it,motion,bone->_LTransform());
}



static void CalculateRest(CBone* bone, Fmatrix& parent)
{
    Fmatrix& R	= bone->_RTransform();
    R.setXYZi	(bone->_RestRotate());
    R.c.set		(bone->_RestOffset());
    bone->_LRTransform() = R;
    R.mulA_43	(parent);
    bone->_RITransform().invert(bone->_RTransform());

    // Calculate children
    for (BoneIt b_it=bone->children.begin(); b_it!=bone->children.end(); b_it++)
    	CalculateRest	(*b_it,bone->_RTransform());
}

void CEditableObject::CalculateAnimation(CSMotion* motion)
{
	if (!m_Bones.empty())
		CalculateAnim(m_Bones.front(),motion,Fidentity);
}
 float CEditableObject::GetBonesBottom()
 {
 	float bottom  = FLT_MAX;
    VERIFY(!m_Bones.empty());
 	for (BoneIt b_it=m_Bones.begin()+1; b_it!=m_Bones.end(); b_it++)
    	if( !(*b_it)->IsRoot() && (*b_it)->_LTransform().c.y < bottom )
        bottom =  (*b_it)->_LTransform().c.y;
    return bottom;
 }

 static void SetBoneTransform( CBone &bone, const Fmatrix &T, const Fmatrix & parent )
{

	bone._LTransform() = T;
    Fmatrix		 LI; LI.invert(parent);
    bone._MTransform().mul_43( LI, T );
	bone._RenderTransform().mul_43(bone._LTransform(),bone._RITransform());
}
bool CEditableObject::AnimateRootObject( CSMotion* motion )
{
  VERIFY( motion );
  if( !motion->m_Flags.test(esmRootMover) )
  	return false;
  CBone	&root_bone 		= *m_Bones[GetRootBoneID()];
  if( root_bone.children.size() != 1 )
  	return false;
  return true;

}
void CEditableObject::GetAnchorForRootObjectAnimation( Fmatrix &anchor )
{
			CBone	&root_bone 		= *m_Bones[GetRootBoneID()];
            VERIFY( root_bone .children.size() == 1 );
			CBone	&anchor_bone 	= *root_bone .children[0];
            anchor =   anchor_bone._LTransform();
            anchor.invert();
}
static void AlineYtoGlobalFrame(Fmatrix &in_out_m)
{
	Fmatrix &m =    in_out_m;
    
    m.i.y = 0;
    m.k.y = 0;

    m.j.x = 0;
    m.j.y = 1;
    m.j.z = 0;

	float     smi = m.i.x  * m.i.x +  m.i.z  * m.i.z;
    float     smk = m.k.x  * m.k.x +  m.k.z  * m.k.z;
    bool bi = smi > EPS_S;
    bool bk = smk > EPS_S;
    if( smk > smi && bk )
    {
		m.k.mul( 1.f/_sqrt( smk ) );
        m.i.crossproduct( m.j, m.k );
    } else if( bi )
    {
    	m.i.mul( 1.f/_sqrt( smi ) );
        m.k.crossproduct( m.i, m.j );
    }else //if( !bi && !bk )
    {
    	//unreal indeed
        m = Fidentity;
    }

//    if		 ( bi && bk )
 //   {
 //		m.i.mul( 1.f/_sqrt( smi ) );
 //       m.k.mul( 1.f/_sqrt( smk ) );
 //   }
    
    if(!check_scale( in_out_m ))
    	VERIFY( check_scale( in_out_m ) );
}
void CEditableObject::CalculateRootObjectAnimation(const Fmatrix &anchor)
{
	float 	bottom =   GetBonesBottom();
			CBone	&root_bone 		= *m_Bones[GetRootBoneID()];
            VERIFY( root_bone .children.size() == 1 );
			CBone	&anchor_bone 	= *root_bone .children[0];
	const	Fmatrix gl_anchor = Fmatrix().mul_43( anchor_bone._LTransform(), anchor );

  	Fmatrix root_transform  = gl_anchor;//gl_anchor;
    AlineYtoGlobalFrame( root_transform );

    root_transform.c =  gl_anchor.c;
    root_transform.c.y=bottom;
    SetBoneTransform(   root_bone, root_transform, Fidentity );
    
 	SetBoneTransform(   anchor_bone, anchor_bone._LTransform(), root_transform );
    
  //  for (BoneIt b_it=root_bone .children.begin(); b_it!=root_bone .children.end(); b_it++)
  //  	SetBoneTransform(   *(*b_it), (*(*b_it))._LTransform(), root_transform );
}
void CEditableObject::CalculateBindPose()
{
	if (!m_Bones.empty())
		CalculateRest(m_Bones.front(),Fidentity);
}

void CEditableObject::SetActiveSMotion(CSMotion* mot)
{
	m_ActiveSMotion=mot;
	if (m_ActiveSMotion) m_SMParam.Set(m_ActiveSMotion);
}

bool CEditableObject::RemoveSMotion(const char* name)
{
    SMotionVec& lst = m_SMotions;
    for(SMotionIt m=lst.begin(); m!=lst.end(); m++)
        if ((stricmp((*m)->Name(),name)==0)){
        	if (m_ActiveSMotion==*m) SetActiveSMotion(0);
            xr_delete(*m);
        	lst.erase(m);
            return true;
        }
    return false;
}                         

//---------------------------------------------------------------------------
/*
bool CEditableObject::LoadSMotions(const char* fname)
{
	IReader* F	= FS.r_open(fname);
    ClearSMotions();
    // object motions
    m_SMotions.resize(F->r_u32());
	SetActiveSMotion(0);
    for (SMotionIt m_it=m_SMotions.begin(); m_it!=m_SMotions.end(); m_it++){
        *m_it = xr_new<CSMotion>();
        if (!(*m_it)->Load(*F)){
            ELog.DlgMsg(mtError,"Motions has different version. Load failed.");
            xr_delete(*m_it);
            m_SMotions.clear();
            FS.r_close(F);
            return false;
        }
	  	if (!CheckBoneCompliance(*m_it)){
        	ClearSMotions();
            ELog.DlgMsg(mtError,"Load failed.",fname);
            xr_delete(&*m_it);
            FS.r_close(F);
            return false;
        }
    }
	FS.r_close(F);
	return true;
}
*/
bool CEditableObject::AppendSMotion(LPCSTR fname, SMotionVec* inserted)
{
	VERIFY(IsSkeleton());

    bool bRes	= true;
    
	LPCSTR ext	= strext(fname);
    if (0==stricmp(ext,".skl")){
        CSMotion* M = xr_new<CSMotion>();
        if (!M->LoadMotion(fname)){
            ELog.Msg(mtError,"Motion '%s' can't load. Append failed.",fname);
            xr_delete(M);
            bRes = false;
        }else{
        	string256 name;
			_splitpath(fname,0,0,name,0);
            if (CheckBoneCompliance(M)){
                M->SortBonesBySkeleton(m_Bones);
                string256 			m_name;
                GenerateSMotionName	(m_name,name,M);
                M->SetName			(m_name);
                m_SMotions.push_back(M);
                if (inserted)		inserted->push_back(M);
                // optimize
                M->Optimize			();
            }else{
                ELog.Msg(mtError,"Append failed.",fname);
                xr_delete(M);
	            bRes = false;
            }
        }
    }else if (0==stricmp(ext,".skls")){
        IReader* F	= FS.r_open(fname);
        if (!F){
        	ELog.Msg(mtError,"Can't open file '%s'.",fname);
            bRes = false;
    	}
        if (bRes){
            // object motions
            int cnt 	= F->r_u32();
            for (int k=0; k<cnt; k++){
                CSMotion* M	= xr_new<CSMotion>();
                if (!M->Load(*F)){
                    ELog.Msg(mtError,"Motion '%s' has different version. Load failed.",M->Name());
                    xr_delete(M);
                    bRes = false;
                    break;
                }
				if (!CheckBoneCompliance(M)){
					xr_delete(M);
					bRes = false;
					break;
				}
                if (bRes){
                    M->SortBonesBySkeleton(m_Bones);
                    string256 			m_name;
                    GenerateSMotionName	(m_name,M->Name(),M);
                    M->SetName			(m_name);
                    m_SMotions.push_back(M);
                    if (inserted)		inserted->push_back(M);
                    // optimize
                    M->Optimize			();
                }
            }
        }
        FS.r_close(F);
    }
    return bRes;
}

void CEditableObject::ClearSMotions()
{
	SetActiveSMotion(0);
    for(SMotionIt m_it=m_SMotions.begin(); m_it!=m_SMotions.end();m_it++)xr_delete(*m_it);
    m_SMotions.clear();
}

bool CEditableObject::SaveSMotions(const char* fname)
{
	CMemoryWriter F;
    F.w_u32		(m_SMotions.size());
    for (SMotionIt m_it=m_SMotions.begin(); m_it!=m_SMotions.end(); m_it++) (*m_it)->Save(F);
    return 		F.save_to(fname);
}

bool CEditableObject::RenameSMotion(const char* old_name, const char* new_name)
{
	if (stricmp(old_name,new_name)==0) return true;
    if (FindSMotionByName(new_name)) return false;
	CSMotion* M = FindSMotionByName(old_name); VERIFY(M);
    M->SetName(new_name);
    return true;
}

CSMotion* CEditableObject::FindSMotionByName	(const char* name, const CSMotion* Ignore)
{
	if (name&&name[0]){
        SMotionVec& lst = m_SMotions;
        for(SMotionIt m=lst.begin(); m!=lst.end(); m++)
            if ((Ignore!=(*m))&&(stricmp((*m)->Name(),name)==0)) return (*m);
    }
    return 0;
}

void CEditableObject::GenerateSMotionName(char* buffer, const char* start_name, const CSMotion* M)
{
	strcpy(buffer,start_name);
    int idx = 0;
	while(FindSMotionByName(buffer,M)){
		sprintf( buffer, "%s_%2d", start_name, idx );
    	idx++;
    }
    strlwr(buffer);
}

ICF bool pred_sort_B(CBone* A, CBone* B)	
{
	return (xr_strcmp(A->Name().c_str(),B->Name().c_str())<0);
}
ICF void fill_bones_by_parent(BoneVec& bones,CBone* start)
{
    bones.push_back		(start);
    for (BoneIt b_it=start->children.begin(); b_it!=start->children.end(); b_it++)
    	fill_bones_by_parent(bones,*b_it);
}
void CEditableObject::PrepareBones()
{
	if (m_Bones.empty())return;
	CBone* PARENT		= 0;
    // clear empty parent
	BoneIt b_it;
    for (b_it=m_Bones.begin(); b_it!=m_Bones.end(); b_it++)
    {
    	(*b_it)->children.clear	();
        (*b_it)->parent			= NULL;
        BoneIt parent	= std::find_if(m_Bones.begin(),m_Bones.end(),fBoneNameEQ((*b_it)->ParentName()));
        if (parent==m_Bones.end()){
        	(*b_it)->SetParentName("");
            VERIFY2		(0==PARENT,"Invalid object. Have more than 1 parent.");
            PARENT		= *b_it;
        }else{
            BoneIt parent	= std::find_if(m_Bones.begin(),m_Bones.end(),fBoneNameEQ((*b_it)->ParentName()));
            CBone* tmp = (parent==m_Bones.end())?0:*parent;
            (*b_it)->parent	= tmp;
        }
    }
    // sort by name
    std::sort(m_Bones.begin(),m_Bones.end(),pred_sort_B);
    // fill children
    for (b_it=m_Bones.begin(); b_it!=m_Bones.end(); b_it++){
        BoneIt parent	= std::find_if(m_Bones.begin(),m_Bones.end(),fBoneNameEQ((*b_it)->ParentName()));
        if (parent!=m_Bones.end()) (*parent)->children.push_back(*b_it);
    }
    // manual sort
    u32 b_cnt			= m_Bones.size();
    m_Bones.clear		();
    fill_bones_by_parent(m_Bones,PARENT);

	u32 cnt_new 		= m_Bones.size();
    VERIFY				(b_cnt==cnt_new);
    // update SelfID
    for (b_it=m_Bones.begin(); b_it!=m_Bones.end(); b_it++)
        (*b_it)->SelfID		= b_it-m_Bones.begin();
    VERIFY(0==m_Bones.front()->parent);
/*    
    for (b_it=m_Bones.begin(); b_it!=m_Bones.end(); b_it++)
    	Msg("%20s - %20s",(*b_it)->Name().c_str(),(*b_it)->ParentName().c_str());
*/
    CalculateBindPose		();
}

BoneIt CEditableObject::FindBoneByNameIt(const char* name)
{
	return std::find_if(m_Bones.begin(),m_Bones.end(),fBoneNameEQ(name));
}

int CEditableObject::FindBoneByNameIdx(LPCSTR name)
{
	BoneIt b_it = FindBoneByNameIt(name);
    return (b_it==m_Bones.end())?-1:b_it-m_Bones.begin();
}

CBone* CEditableObject::FindBoneByName(const char* name)
{
	BoneIt b_it = FindBoneByNameIt(name);
    return (b_it==m_Bones.end())?0:*b_it;
}

int CEditableObject::GetRootBoneID()
{
    for (BoneIt b_it=m_Bones.begin(); b_it!=m_Bones.end(); b_it++)
    	if ((*b_it)->IsRoot()) return b_it-m_Bones.begin();
    THROW;
    return -1;
}

int CEditableObject::PartIDByName(LPCSTR name)
{
	for (BPIt it=m_BoneParts.begin(); it!=m_BoneParts.end(); it++)
    	if (it->alias==name) return it-m_BoneParts.begin();
    return -1;
} 

shared_str CEditableObject::BoneNameByID(int id)
{
	VERIFY((id>=0)&&(id<(int)m_Bones.size()));
    return m_Bones[id]->Name();
}

u16	CEditableObject::GetBoneIndexByWMap(const char* wm_name)
{
	BoneIt bone = std::find_if(m_Bones.begin(),m_Bones.end(),fBoneWMNameEQ(wm_name));
    return (bone==m_Bones.end())?BI_NONE:(u16)(bone-m_Bones.begin());
}

void CEditableObject::GetBoneWorldTransform(u32 bone_idx, float t, CSMotion* motion, Fmatrix& matrix)
{
	VERIFY(bone_idx<m_Bones.size());
    int idx	= bone_idx;
    matrix.identity();
    IntVec lst;
    do{ lst.push_back(idx); }while((idx=m_Bones[idx]->Parent()?m_Bones[idx]->Parent()->SelfID:-1)>-1);
    for (int i=lst.size()-1; i>=0; i--){
    	idx = lst[i];
	    Flags8 flags	= motion->GetMotionFlags(idx);
    	Fvector T,R;
        Fmatrix rot, mat;
        motion->_Evaluate(idx,t,T,R);
        if (flags.is(st_BoneMotion::flWorldOrient)){
            rot.setXYZi(R.x,R.y,R.z);
            mat.identity();
            mat.c.set(T);
            mat.mulA_43(matrix);
            mat.i.set(rot.i);
            mat.j.set(rot.j);
            mat.k.set(rot.k);
        }else{
            mat.setXYZi(R.x,R.y,R.z);
            mat.c.set(T);
            mat.mulA_43(matrix);
        }
        matrix.set(mat);
    }
}

bool CEditableObject::CheckBoneCompliance(CSMotion* M)
{
	VERIFY(M);
/*
    BoneMotionVec& lst = M->BoneMotions();
	if (m_Bones.size()!=lst.size()){
		Log		("!Different bone count.\nObject has '%d' bones. Motion has '%d' bones.",m_Bones.size(),lst.size());
    	return false;
    }
    for(BoneMotionIt bm_it=lst.begin(); bm_it!=lst.end(); bm_it++)
    	if (!FindBoneByName(*bm_it->name)){
        	Msg		("!Can't find bone '%s' in object.",bm_it->name);
        	return false;
        }
*/
    for(BoneIt b_it=m_Bones.begin(); b_it!=m_Bones.end(); b_it++)
    	if (!M->FindBoneMotion((*b_it)->Name())) {
//        	Msg		("!Can't find bone '%s' in motion.",*(*b_it)->Name());
//        	return false;
			M->add_empty_motion	((*b_it)->Name());
			continue;
        }
    return true;
}

void CEditableObject::OptimizeSMotions()
{
#ifdef _EDITOR
	SPBItem* pb				= UI->ProgressStart(m_SMotions.size(),"Motions optimizing...");
#endif
	for (SMotionIt s_it=m_SMotions.begin(); s_it!=m_SMotions.end(); s_it++){
        (*s_it)->Optimize	();
#ifdef _EDITOR
		pb->Inc				();
#endif
	}
#ifdef _EDITOR
    UI->ProgressEnd				(pb);
#endif
}
