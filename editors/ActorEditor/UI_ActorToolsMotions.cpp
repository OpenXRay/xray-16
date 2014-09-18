//---------------------------------------------------------------------------

#include "stdafx.h"
#pragma hdrstop

#include "UI_ActorTools.h"
#include "../ECore/Editor/UI_Main.h"
#include "leftbar.h"
#include "../ECore/Editor/EditObject.h"
#include "../xrEProps/PropertiesList.h"
#include "motion.h"
#include "bone.h"
#include "../../Layers/xrRender/SkeletonAnimated.h"
#include "fmesh.h"
#include "../xrEProps/folderlib.h"
#include "leftbar.h"
#include "../xrEProps/ItemList.h"
#include "../../xrphysics/physicsshell.h"
//---------------------------------------------------------------------------
MotionID EngineModel::FindMotionID(LPCSTR name, u16 slot)
{
	MotionID M;
	CKinematicsAnimated* VA = PKinematicsAnimated(m_pVisual);
    if (VA) M				= VA->ID_Motion	(name,slot);
    return M;
}
CMotionDef*	EngineModel::FindMotionDef(LPCSTR name, u16 slot)
{
	CKinematicsAnimated* VA 	= PKinematicsAnimated(m_pVisual);
    if (VA){
        MotionID M				= FindMotionID(name,slot);
        if (M.valid())			return VA->LL_GetMotionDef(M);
    }
    return 0;
}
CMotion*	EngineModel::FindMotionKeys(LPCSTR name, u16 slot)
{
	CKinematicsAnimated* VA 	= PKinematicsAnimated(m_pVisual);
    if (VA){
    	MotionID M				= FindMotionID(name,slot);
        if (M.valid())			return VA->LL_GetMotion	(M,VA->LL_GetBoneRoot());
    }
    return 0;
}

void EngineModel::FillMotionList(LPCSTR pref, ListItemsVec& items, int modeID)
{
    LHelper().CreateItem			(items, pref,  modeID, 0);
    if (IsRenderable()&&fraLeftBar->ebRenderEngineStyle->Down){
    	CKinematicsAnimated* SA		= PKinematicsAnimated(m_pVisual);
		if (SA){
            for (int k=SA->m_Motions.size()-1; k>=0; --k){
            	xr_string slot_pref	= ATools->BuildMotionPref((u16)k,pref);
			    LHelper().CreateItem(items, slot_pref.c_str(),  modeID, ListItem::flSorted);
	            // cycles
                accel_map::const_iterator I,E;
                I = SA->m_Motions[k].motions.cycle()->begin(); 
                E = SA->m_Motions[k].motions.cycle()->end();              
                for ( ; I != E; ++I){
                	shared_str tmp = PrepareKey(slot_pref.c_str(),*(*I).first);
                    LHelper().CreateItem(items, tmp.c_str(), modeID, 0, *(void**)&MotionID((u16)k,I->second));
            	}
                // fxs
                I = SA->m_Motions[k].motions.fx()->begin(); 
                E = SA->m_Motions[k].motions.fx()->end(); 
                for ( ; I != E; ++I){
                	shared_str tmp = PrepareKey(slot_pref.c_str(),*(*I).first);
                    LHelper().CreateItem(items, tmp.c_str(), modeID, 0, *(void**)&MotionID((u16)k,I->second));
                }
            }
        }
    }
}
/*
void EngineModel::PlayCycle(LPCSTR name, int part, u16 slot)
{
    MotionID D = PKinematicsAnimated(m_pVisual)->ID_Motion(name,slot);
    if (D.valid())
        PKinematicsAnimated(m_pVisual)->LL_PlayCycle((u16)part,D,TRUE,0,0);
}

void EngineModel::PlayFX(LPCSTR name, float power, u16 slot)
{
    MotionID D = PKinematicsAnimated(m_pVisual)->ID_Motion(name,slot);
    if (D.valid())
    	PKinematicsAnimated(m_pVisual)->PlayFX(D,power);
}
*/

void EngineModel::StopAnimation()
{
    if (m_pVisual &&  PKinematicsAnimated(m_pVisual) ){
        PKinematicsAnimated(m_pVisual)->LL_CloseCycle(0);
        PKinematicsAnimated(m_pVisual)->LL_CloseCycle(1);
        PKinematicsAnimated(m_pVisual)->LL_CloseCycle(2);
        PKinematicsAnimated(m_pVisual)->LL_CloseCycle(3);
    }
}

bool EngineModel::UpdateGeometryStream(CEditableObject* source)
{
	m_GeometryStream.clear();
    if (!source) return false;
    if (source->IsSkeleton())	return (source->PrepareSVGeometry(m_GeometryStream,4));
    else						return (source->PrepareOGF(m_GeometryStream,4,true,NULL));
}

bool EngineModel::UpdateMotionDefsStream(CEditableObject* source)
{
	m_MotionDefsStream.clear();
	return (source&&source->PrepareSVDefs(m_MotionDefsStream));
}

bool EngineModel::UpdateMotionKeysStream(CEditableObject* source)
{
	m_MotionKeysStream.clear();
	return (source&&source->PrepareSVKeys(m_MotionKeysStream));
}

bool EngineModel::UpdateVisual(CEditableObject* source, bool bUpdGeom, bool bUpdKeys, bool bUpdDefs)
{
	bool bRes = true;
	CMemoryWriter F;
    destroy_physics_shell( m_physics_shell );
	if (source->IsSkeleton()){
        if (bUpdGeom)	bRes = UpdateGeometryStream(source);
        if (!bRes||!m_GeometryStream.size()){
        	ELog.Msg(mtError,"Can't create preview geometry.");
        	return false;
        }
        F.w(m_GeometryStream.pointer(),m_GeometryStream.size());
        if (bUpdKeys) UpdateMotionKeysStream(source);
        if (bUpdDefs) UpdateMotionDefsStream(source);
        if (m_MotionKeysStream.size())	F.w(m_MotionKeysStream.pointer(),m_MotionKeysStream.size());
        if (m_MotionDefsStream.size())	F.w(m_MotionDefsStream.pointer(),m_MotionDefsStream.size());
    }else{
        bool bRes = true;
        if (bUpdGeom) 	bRes = UpdateGeometryStream(source);
        if (!bRes){
        	ELog.Msg(mtError,"Can't create preview geometry.");
        	return false;
        }
        if (!m_GeometryStream.size()) return false;
        F.w(m_GeometryStream.pointer(),m_GeometryStream.size());
    }
    IReader R							(F.pointer(), F.size());
    ::Render->model_Delete				(m_pVisual,TRUE);
    g_pMotionsContainer->clean			(false);
    m_pVisual = ::Render->model_Create	(ChangeFileExt(source->GetName(),"").c_str(),&R);
    m_pBlend = 0;
    return bRes;
}

//---------------------------------------------------------------------------

void EngineModel::PlayMotion(LPCSTR name, u16 slot)
{
    for (int k=0; k<MAX_PARTS; k++)
       m_BPPlayItems[k].name = "";

    StopAnimation();

    CKinematicsAnimated* SA 	= PKinematicsAnimated(m_pVisual);
	if (IsRenderable()&&SA){
        MotionID motion_ID 		= FindMotionID(name, slot);
        if (motion_ID.valid()){
            CMotionDef* mdef 	= SA->LL_GetMotionDef(motion_ID); VERIFY(mdef);
            if (mdef->flags&esmFX)
            {
                for (int k=0; k<MAX_PARTS; k++)
                {
                    if (!m_BPPlayItems[k].name.IsEmpty())
                    {
                        MotionID D 		= SA->ID_Motion(m_BPPlayItems[k].name.c_str(),m_BPPlayItems[k].slot);
                        if (D.valid()) 	SA->LL_PlayCycle((u16)k,D,false,0,0);
                    }
                }        
                m_pBlend = SA->PlayFX(motion_ID,1.f);
            }else{	
                u16 idx 		= mdef->bone_or_part;
                R_ASSERT((idx==BI_NONE)||(idx<MAX_PARTS));
                if (BI_NONE==idx)
                {
                	for (int k=0; k<MAX_PARTS; k++){ 
                		m_BPPlayItems[k].name 	= name;
	                    m_BPPlayItems[k].slot	= slot;
                    }
                }else{	
	                m_BPPlayItems[idx].name		= name;
					m_BPPlayItems[idx].slot		= slot;
                }
                m_pBlend		= 0;

                for (int k=0; k<MAX_PARTS; k++)
                {
                    if (!m_BPPlayItems[k].name.IsEmpty())
                    {
                        MotionID D 	= SA->ID_Motion(m_BPPlayItems[k].name.c_str(),m_BPPlayItems[k].slot);
                        CBlend* B	= 0;
                        if (D.valid())
                        {
                            B = SA->LL_PlayCycle((u16)k,D,false,0,0);
                            if(B && (idx==k || idx==BI_NONE) ) 
                            	m_pBlend = B;
                        }
                    }
                }        
            }
        }
    }
/*
    if (M&&IsRenderable()){
        if (M->flags&esmFX){
			for (int k=0; k<MAX_PARTS; k++){
            	if (!m_BPPlayCache[k].IsEmpty()){
                	CMotionDef* D = PSkeletonAnimated(m_pVisual)->ID_Cycle_Safe(m_BPPlayCache[k].c_str());
                    if (D) D->PlayCycle(PSkeletonAnimated(m_pVisual),k,false,0,0);
    	    	}
            }        
        	m_pBlend = PSkeletonAnimated(m_pVisual)->PlayFX(M->Name(),1.f);
        }else{	
        	R_ASSERT((M->m_BoneOrPart==BI_NONE)||(M->m_BoneOrPart<MAX_PARTS));
            u16 idx 		= M->m_BoneOrPart;
        	if (BI_NONE==idx)for (int k=0; k<MAX_PARTS; k++) m_BPPlayCache[k] = M->Name();
            else			m_BPPlayCache[idx] = M->Name();
            m_pBlend		= 0;

			for (int k=0; k<MAX_PARTS; k++){
            	if (!m_BPPlayCache[k].IsEmpty()){
                	CMotionDef* D = PSkeletonAnimated(m_pVisual)->ID_Cycle_Safe(m_BPPlayCache[k].c_str());
                    CBlend* B=0;
                    if (D){
                    	B = D->PlayCycle(PSkeletonAnimated(m_pVisual),k,(idx==k)?!(D->flags&esmNoMix):FALSE,0,0);
						if (idx==k) m_pBlend = B;
                    }
    	    	}
            }        
        }
    }
*/
}
void EngineModel::RestoreParams(TFormStorage* s)
{          
    for (u16 k=0; k<MAX_PARTS; k++){
    	m_BPPlayItems[k].name	= s->ReadString("bp_cache_name_"+AnsiString(k),"");
    	m_BPPlayItems[k].slot	= (u16)s->ReadInteger("bp_cache_slot_"+AnsiString(k),0);
    }
}

void EngineModel::SaveParams(TFormStorage* s)
{
    for (int k=0; k<MAX_PARTS; k++){
	    s->WriteString	("bp_cache_name_"+AnsiString(k),	m_BPPlayItems[k].name);
	    s->WriteString	("bp_cache_slot_"+AnsiString(k),	m_BPPlayItems[k].slot);
    }
}

//---------------------------------------------------------------------------

void CActorTools::OnMotionKeysModified()
{
	Modified			();
	m_Flags.set			(flUpdateMotionKeys,TRUE);
    if (fraLeftBar->ebRenderEngineStyle->Down){
		m_Flags.set		(flUpdateMotionKeys,FALSE);
        if (m_RenderObject.UpdateVisual(m_pEditObject,false,true,false)){
            PlayMotion();
        }else{
            m_RenderObject.DeleteVisual();
            fraLeftBar->SetRenderStyle(false);
        }
    }
    OnMotionDefsModified();
}

void CActorTools::OnMotionDefsModified()
{
	Modified			();
	m_Flags.set			(flUpdateMotionDefs,TRUE);
    if (fraLeftBar->ebRenderEngineStyle->Down){
		m_Flags.set		(flUpdateMotionDefs,FALSE);
        if (m_RenderObject.UpdateVisual(m_pEditObject,false,false,true)){
            PlayMotion();
        }else{
            m_RenderObject.DeleteVisual();
            fraLeftBar->SetRenderStyle(false);
        }
    }
    UndoSave			();
}

void CActorTools::OnGeometryModified()
{
	Modified			();
    if (fraLeftBar->ebRenderEngineStyle->Down){
		m_Flags.set		(flUpdateGeometry,FALSE);
        if (m_RenderObject.UpdateVisual(m_pEditObject,true,false,false)){
            PlayMotion();
        }else{
            m_RenderObject.DeleteVisual();
            fraLeftBar->SetRenderStyle(false);
        }
    }
    UndoSave			();  
}
//---------------------------------------------------------------------------

bool CActorTools::AppendMotion(LPCSTR fn)
{
	VERIFY(m_pEditObject);
    bool bRes = m_pEditObject->AppendSMotion(fn,&appended_motions);
    return bRes;
}

bool CActorTools::RemoveMotion(LPCSTR name)
{
	VERIFY(m_pEditObject);
    return m_pEditObject->RemoveSMotion(name);
}

bool CActorTools::SaveMotions(LPCSTR name, bool bSelOnly)
{
	VERIFY(m_pEditObject);
    ListItemsVec items;
    if (bSelOnly){
        if (m_ObjectItems->GetSelected(MOTIONS_PREFIX,items,true)){
            CMemoryWriter 	F;
            F.w_u32			(items.size());
            for (ListItemsIt it=items.begin(); it!=items.end(); it++)
                ((CSMotion*)(*it)->m_Object)->Save(F);
            return F.save_to(name);
        }
    }else{
    	return m_pEditObject->SaveSMotions(name);
    }
    return false;
}

void CActorTools::MakePreview()
{
	if (m_pEditObject){
        CMemoryWriter F;
		m_Flags.set		(flUpdateGeometry|flUpdateMotionDefs|flUpdateMotionKeys,FALSE);
    	if (m_RenderObject.UpdateVisual(m_pEditObject,true,true,true)){
            PlayMotion();
        }else{
        	m_RenderObject.DeleteVisual();
	        fraLeftBar->SetRenderStyle(false);
        }
    }else{
    	ELog.DlgMsg(mtError,"Scene empty. Load object first.");
    }
}

void CActorTools::PlayMotion()
{
	if (m_pEditObject){
//.	    m_ClipMaker->Stop();
    	if (fraLeftBar->ebRenderEditorStyle->Down) m_pEditObject->SkeletonPlay();
        else if (fraLeftBar->ebRenderEngineStyle->Down) {
        	if (m_Flags.is(flUpdateMotionKeys))	{ OnMotionKeysModified();	}
        	if (m_Flags.is(flUpdateMotionDefs))	{ OnMotionDefsModified(); 	}
        	if (m_Flags.is(flUpdateGeometry))	{ OnGeometryModified(); 	}
            m_RenderObject.PlayMotion(m_CurrentMotion.c_str(),m_CurrentSlot);
        }
    }
}

void CActorTools::StopMotion()
{
	if (m_pEditObject)
    	if (fraLeftBar->ebRenderEditorStyle->Down) m_pEditObject->SkeletonStop();
        else if (fraLeftBar->ebRenderEngineStyle->Down&&m_RenderObject.m_pBlend) {
        	m_RenderObject.m_pBlend->playing	 = false;
        	m_RenderObject.m_pBlend->timeCurrent = 0;
        }
}

void CActorTools::PauseMotion()
{
	if (m_pEditObject)
    	if (fraLeftBar->ebRenderEditorStyle->Down) m_pEditObject->SkeletonPause(true);
        else if (fraLeftBar->ebRenderEngineStyle->Down&&m_RenderObject.m_pBlend) {
        	m_RenderObject.m_pBlend->playing=!m_RenderObject.m_pBlend->playing;
        }
}

bool CActorTools::RenameMotion(LPCSTR old_name, LPCSTR new_name)
{
	R_ASSERT(m_pEditObject);
	CSMotion* M = m_pEditObject->FindSMotionByName(old_name);	R_ASSERT(M);
	CSMotion* MN = m_pEditObject->FindSMotionByName(new_name);	R_ASSERT(!MN);
    M->SetName(new_name);
    return true;
}


void CActorTools::AddMarksChannel(bool b12)
{
    CSMotion* M 		= m_pEditObject->GetActiveSMotion();
    if(M)
    {
    	if(b12)
        {
            M->marks.resize					(2);
            M->marks[0].name				= "Left";
            M->marks[1].name				= "Right";
		}else
        {
            M->marks.resize					(4);
            M->marks[0].name				= "Left";
            M->marks[1].name				= "Right";
            M->marks[2].name				= "Left2";
            M->marks[3].name				= "Right2";
        }
        m_pEditObject->SetActiveSMotion(M);
    }
}

void CActorTools::RemoveMarksChannel(bool b12)
{
    CSMotion* M 					= m_pEditObject->GetActiveSMotion();
    if(M)
    {
        if(b12)
        	M->marks.clear			();
        else
        {
            if(M->marks.size()==4)
            {
                M->marks.pop_back();
                M->marks.pop_back();
            }else
            {
            	R_ASSERT(M->marks.size()==0 || M->marks.size()==2);
            }
        }

        ExecCommand					(COMMAND_UPDATE_PROPERTIES);
    }
}
