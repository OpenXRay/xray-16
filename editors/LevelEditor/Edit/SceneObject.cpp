//----------------------------------------------------
// file: CSceneObject.cpp
//----------------------------------------------------

#include "stdafx.h"
#pragma hdrstop

#include "SceneObject.h"
#include "bottombar.h"
#include "../ECore/Editor/library.h"
#include "../ECore/Editor/EditMesh.h"
#include "../ECore/Editor/ui_main.h"
#include "../ECore/Editor/D3DUtils.h"

#ifdef _LEVEL_EDITOR
	#include "scene.h"
#endif


#define BLINK_TIME 300.f

//----------------------------------------------------
CSceneObject::CSceneObject(LPVOID data, LPCSTR name):CCustomObject(data,name)
{
	Construct	(data);
}

void CSceneObject::Construct(LPVOID data)
{
	ClassID		= OBJCLASS_SCENEOBJECT;

    m_ReferenceName = "";
	m_pReference 	= 0;

    m_TBBox.invalidate();
    m_iBlinkTime	= 0;
    m_BlinkSurf		= 0;

    m_Flags.zero	();
}

CSceneObject::~CSceneObject()
{
	Lib.RemoveEditObject(m_pReference);
}
//----------------------------------------------------

void CSceneObject::EvictObject()
{
	if (m_pReference) m_pReference->EvictObject();
}

//----------------------------------------------------
void CSceneObject::Select(BOOL flag)
{
	inherited::Select(flag);
    if (flag) Blink();
}

//----------------------------------------------------
int CSceneObject::GetFaceCount()
{
	return m_pReference?m_pReference->GetFaceCount():0;
}

int CSceneObject::GetSurfFaceCount(const char* surf_name)
{
	return m_pReference?m_pReference->GetSurfFaceCount(surf_name):0;
}

int CSceneObject::GetVertexCount()
{
	return m_pReference?m_pReference->GetVertexCount():0;
}

void CSceneObject::OnUpdateTransform()
{
	inherited::OnUpdateTransform();
    // update bounding volume
    if (m_pReference){
    	m_TBBox.set		(m_pReference->GetBox());
    	m_TBBox.xform	(_Transform());
    }
}

bool CSceneObject::GetBox( Fbox& box ) const
{
	if (!m_pReference) return false;
	box.set(m_TBBox);
	return true;
}

bool CSceneObject::GetUTBox( Fbox& box )
{
	if (!m_pReference) return false;
    box.set(m_pReference->GetBox());
	return true;
}

bool CSceneObject::IsRender()
{
	if (!m_pReference) return false;
    return inherited::IsRender();
}


void CSceneObject::Render(int priority, bool strictB2F)
{
	inherited::Render(priority,strictB2F);
    if (!m_pReference) return;
#ifdef _LEVEL_EDITOR    
    Scene->SelectLightsForObject(this);
#endif
	m_pReference->Render(_Transform(), priority, strictB2F);
    if (Selected()){
    	if (1==priority){
            if (false==strictB2F){
                EDevice.SetShader(EDevice.m_WireShader);
                RCache.set_xform_world(_Transform());
                u32 clr = 0xFFFFFFFF;
                DU_impl.DrawSelectionBoxB(m_pReference->GetBox(),&clr);
            }else{
                RenderBlink	();
            }
        }
    }
}

void CSceneObject::RenderBlink()
{
    if (m_iBlinkTime>0){
        if (m_iBlinkTime>(int)EDevice.dwTimeGlobal){
        	int alpha = iFloor(sqrtf(float(m_iBlinkTime-EDevice.dwTimeGlobal)/BLINK_TIME)*64);
			m_pReference->RenderSelection(_Transform(),0, m_BlinkSurf, D3DCOLOR_ARGB(alpha,255,255,255));
            UI->RedrawScene	();
        }else{
            m_iBlinkTime 	= 0;
            m_BlinkSurf		= 0;
        }
    }
}

void CSceneObject::RenderSingle()
{
	if (!m_pReference) 		return;
	m_pReference->RenderSingle(_Transform());
    RenderBlink				();
}

void CSceneObject::RenderBones()
{
	if (!m_pReference) return;
	m_pReference->RenderBones(_Transform());
}

void CSceneObject::RenderEdge(CEditableMesh* mesh, u32 color)
{
	if (!m_pReference) return;
    if (::Render->occ_visible(m_TBBox))
		m_pReference->RenderEdge(_Transform(), mesh, 0, color);
}

void CSceneObject::RenderSelection(u32 color)
{
	if (!m_pReference) return;
	m_pReference->RenderSelection(_Transform(),0, 0, color);
}

bool CSceneObject::FrustumPick(const CFrustum& frustum)
{
	if (!m_pReference) return false;
    if (::Render->occ_visible(m_TBBox))
		return m_pReference->FrustumPick(frustum, _Transform());
    return false;
}

bool CSceneObject::SpherePick(const Fvector& center, float radius)
{
	if (!m_pReference) return false;
    float fR; Fvector vC;
	m_TBBox.getsphere(vC,fR);
	float R=radius+fR;
    float dist_sqr=center.distance_to_sqr(vC);
    if (dist_sqr<R*R) return true;
    return false;
}

bool CSceneObject::RayPick(float& dist, const Fvector& S, const Fvector& D, SRayPickInfo* pinf)
{
	if (!m_pReference) return false;
    if (::Render->occ_visible(m_TBBox))
		if (m_pReference->RayPick(dist, S, D, _ITransform(), pinf)){
        	if (pinf) pinf->s_obj = this;
            return true;
        }
	return false;
}

void CSceneObject::RayQuery(SPickQuery& pinf)
{
	if (!m_pReference) return;
    m_pReference->RayQuery(_Transform(), _ITransform(), pinf);
}

void CSceneObject::BoxQuery(SPickQuery& pinf)
{
	if (!m_pReference) return;
    m_pReference->BoxQuery(_Transform(), _ITransform(), pinf);
}

bool CSceneObject::BoxPick(const Fbox& box, SBoxPickInfoVec& pinf)
{
	if (!m_pReference) return false;
	return m_pReference->BoxPick(this, box, _ITransform(), pinf);
}

void CSceneObject::GetFullTransformToWorld( Fmatrix& m )
{
    m.set(_Transform());
}

void CSceneObject::GetFullTransformToLocal( Fmatrix& m )
{
    m.set(_ITransform());
}

CEditableObject* CSceneObject::UpdateReference()
{
	Lib.RemoveEditObject(m_pReference);
	m_pReference		= (m_ReferenceName.size())?Lib.CreateEditObject(*m_ReferenceName):0;
    UpdateTransform		();
    return m_pReference;
}

CEditableObject* CSceneObject::SetReference(LPCSTR ref_name)
{
	m_ReferenceName	= ref_name;
    return UpdateReference();
}

void CSceneObject::OnFrame()
{
	inherited::OnFrame();
	if (!m_pReference) return;
	if (m_pReference) m_pReference->OnFrame();
	if (psDeviceFlags.is(rsStatistic)){
    	if (IsStatic()||IsMUStatic()||Selected()){
            EDevice.Statistic->dwLevelSelFaceCount 	+= GetFaceCount();
            EDevice.Statistic->dwLevelSelVertexCount += GetVertexCount();
        }
    }
}

void CSceneObject::ReferenceChange(PropValue* sender)
{
    Scene->BeforeObjectChange(this);
	UpdateReference	();
}

void CSceneObject::FillProp(LPCSTR pref, PropItemVec& items)
{
	inherited::FillProp	(pref,items);
    PropValue* V		= PHelper().CreateChoose	(items,PrepareKey(pref,"Reference"),		&m_ReferenceName, smObject); 
    V->OnChangeEvent.bind(this,&CSceneObject::ReferenceChange);
    if (IsDynamic())
	    inherited::AnimationFillProp(pref,items);
}

bool CSceneObject::GetSummaryInfo(SSceneSummary* inf)
{
	inherited::GetSummaryInfo	(inf);
	CEditableObject* E 	= GetReference(); R_ASSERT(E);
	if (IsStatic()||IsMUStatic()){
        for(SurfaceIt 	s_it=E->m_Surfaces.begin(); s_it!=E->m_Surfaces.end(); s_it++){
			float area			= 0.f;
			float pixel_area	= 0.f;
            for(EditMeshIt m = E->Meshes().begin();m!=E->Meshes().end();m++){
            	area			+= (*m)->CalculateSurfaceArea(*s_it,true);
                pixel_area		+= (*m)->CalculateSurfacePixelArea(*s_it,true);
            }
            inf->AppendTexture(ChangeFileExt(AnsiString(*(*s_it)->m_Texture),"").LowerCase().c_str(),SSceneSummary::sttBase,area,pixel_area,E->m_LibName.c_str());
        }
        if (m_Flags.is(CEditableObject::eoUsingLOD)){
            inf->AppendTexture(E->GetLODTextureName().c_str(),SSceneSummary::sttLOD,0,0,"$LOD$");
            inf->lod_objects.insert	(E->m_LibName.c_str());
            inf->object_lod_ref_cnt++;
        }
        if (m_Flags.is(CEditableObject::eoMultipleUsage)){
            inf->mu_objects.insert(E->m_LibName.c_str());
            inf->object_mu_ref_cnt++;
        }

        inf->face_cnt		+= E->GetFaceCount	();
        inf->vert_cnt		+= E->GetVertexCount();
    }
	if (m_Flags.is(CEditableObject::eoHOM)){
    	inf->hom_face_cnt	+= E->GetFaceCount	();
    	inf->hom_vert_cnt	+= E->GetVertexCount();
    }
    if (m_Flags.is(CEditableObject::eoSoundOccluder)){
    	inf->snd_occ_face_cnt += E->GetFaceCount();
    	inf->snd_occ_vert_cnt += E->GetVertexCount();
    }
    inf->AppendObject	(E->GetName());
	return true;
}

extern xr_token ECORE_API eo_type_token[];

void CSceneObject::OnShowHint(AStringVec& dest)
{
	inherited::OnShowHint(dest);
    dest.push_back(AnsiString("Reference: ")+*m_ReferenceName);
    dest.push_back(AnsiString("-------"));
    float dist			= UI->ZFar();
    SRayPickInfo pinf;
    if (m_pReference->RayPick(dist,UI->m_CurrentRStart,UI->m_CurrentRDir,_ITransform(),&pinf)){
	    dest.push_back(AnsiString("Object Type: ")+get_token_name(eo_type_token,pinf.e_obj->m_objectFlags.flags));
        R_ASSERT(pinf.e_mesh);
        CSurface* surf=pinf.e_mesh->GetSurfaceByFaceID(pinf.inf.id);
        dest.push_back(AnsiString("Surface: ")+AnsiString(surf->_Name()));
        dest.push_back(AnsiString("2 Sided: ")+AnsiString(surf->m_Flags.is(CSurface::sf2Sided)?"on":"off"));
        if (pinf.e_obj->m_objectFlags.is(CEditableObject::eoSoundOccluder)){
	        dest.push_back(AnsiString("Game Mtl: ")+AnsiString(surf->_GameMtlName()));
            int gm_id			= surf->_GameMtl(); 
            if (gm_id!=GAMEMTL_NONE_ID){ 
                SGameMtl* mtl 	= GMLib.GetMaterialByID(gm_id);
                if (mtl)		dest.push_back(AnsiString().sprintf("Occlusion Factor: %3.2f",mtl->fSndOcclusionFactor));
            }
        }else if (pinf.e_obj->m_objectFlags.is(CEditableObject::eoHOM)){
        }else{
	        dest.push_back(AnsiString("Texture: ")+AnsiString(surf->_Texture()));
    	    dest.push_back(AnsiString("Shader: ")+AnsiString(surf->_ShaderName()));
        	dest.push_back(AnsiString("LC Shader: ")+AnsiString(surf->_ShaderXRLCName()));
	        dest.push_back(AnsiString("Game Mtl: ")+AnsiString(surf->_GameMtlName()));
        }
    }
}
//----------------------------------------------------

void CSceneObject::Blink(CSurface* surf)
{
	m_BlinkSurf		= surf;
    m_iBlinkTime	= EDevice.dwTimeGlobal+BLINK_TIME+EDevice.dwTimeDelta;
}
//----------------------------------------------------

bool CSceneObject::Validate(bool bMsg)
{
	CEditableObject* E 	= GetReference(); R_ASSERT(E);
    return E->Validate();
}
//----------------------------------------------------


