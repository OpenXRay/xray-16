#include "stdafx.h"
#pragma hdrstop

#include "ESceneCustomOTools.h"
#include "../ECore/Editor/ui_main.h"
#include "scene.h"
#include "CustomObject.h"

ESceneCustomOTool::ESceneCustomOTool(ObjClassID cls):ESceneToolBase(cls)
{
}
//----------------------------------------------------

ESceneCustomOTool::~ESceneCustomOTool()
{
}
//----------------------------------------------------

void ESceneCustomOTool::UpdateSnapList()
{
}
//----------------------------------------------------

ObjectList*	ESceneCustomOTool::GetSnapList()
{
	return 0;
}
//----------------------------------------------------

BOOL ESceneCustomOTool::_AppendObject(CCustomObject* object)
{
    m_Objects.push_back(object);
    object->ParentTool = this;
    return TRUE;
}
//----------------------------------------------------

BOOL ESceneCustomOTool::_RemoveObject(CCustomObject* object)
{
    object->OnSceneRemove();
	m_Objects.remove(object);
    return FALSE;
}
//----------------------------------------------------

void ESceneCustomOTool::Clear(bool bInternal)
{
	inherited::Clear	();
	for (ObjectIt it=m_Objects.begin(); it!=m_Objects.end(); it++)
    {
    	(*it)->OnSceneRemove();
    	xr_delete(*it);
    }
    m_Objects.clear();
}

BOOL  ESceneCustomOTool::AllowMouseStart()
{
	for(ObjectIt it=m_Objects.begin(); it!=m_Objects.end(); ++it)
    {
    	CCustomObject* CO = *it;
       if(CO->Selected() && !CO->Editable() )
       	return FALSE;
    }
       
    return TRUE;
}

void ESceneCustomOTool::OnFrame()
{
	ObjectList remove_objects;
	for (ObjectIt it=m_Objects.begin(); it!=m_Objects.end(); it++)
    {
    	R_ASSERT				(*it);
    	(*it)->OnFrame			();
        if ((*it)->IsDeleted())	
        	remove_objects.push_back(*it);
    }
    bool need_undo = remove_objects.size();
    while (!remove_objects.empty()){
    	CCustomObject* O	= remove_objects.back();
        Scene->RemoveObject	(O,false,true);
        xr_delete			(O);
        remove_objects.pop_back();
    }
    if (need_undo) Scene->UndoSave();
}
//----------------------------------------------------

void ESceneCustomOTool::OnRender(int priority, bool strictB2F)
{
//	for (ObjectIt it=m_Objects.begin(); it!=m_Objects.end(); it++)
//    	(*it)->Render(priority,strictB2F);
}
//----------------------------------------------------

void ESceneCustomOTool::OnSynchronize()
{
	for (ObjectIt it=m_Objects.begin(); it!=m_Objects.end(); it++)
    	(*it)->OnSynchronize();
}
//----------------------------------------------------

void ESceneCustomOTool::OnSceneUpdate()
{
	for (ObjectIt it=m_Objects.begin(); it!=m_Objects.end(); it++)
    	(*it)->OnSceneUpdate();
}
//----------------------------------------------------

void ESceneCustomOTool::OnDeviceCreate()
{
	for (ObjectIt it=m_Objects.begin(); it!=m_Objects.end(); it++)
    	(*it)->OnDeviceCreate();
}
//----------------------------------------------------

void ESceneCustomOTool::OnDeviceDestroy()
{
	for (ObjectIt it=m_Objects.begin(); it!=m_Objects.end(); it++)
    	(*it)->OnDeviceDestroy();
}
//----------------------------------------------------

bool ESceneCustomOTool::Validate(bool)
{
	for (ObjectIt it=m_Objects.begin(); it!=m_Objects.end(); it++)
    	if (!(*it)->Validate(true)) return false;
    return true;
}

bool ESceneCustomOTool::Valid()
{
	return true;
}

bool ESceneCustomOTool::IsNeedSave()
{
	return !m_Objects.empty();
}

void ESceneCustomOTool::OnObjectRemove(CCustomObject* O, bool bDeleting)
{
    for(ObjectIt _F = m_Objects.begin();_F!=m_Objects.end();_F++)
        (*_F)->OnObjectRemove(O);
}

void ESceneCustomOTool::SelectObjects(bool flag)
{
    for(ObjectIt _F = m_Objects.begin();_F!=m_Objects.end();_F++)
        if((*_F)->Visible()){
            (*_F)->Select( flag );
        }
    UI->RedrawScene		();
}

void ESceneCustomOTool::RemoveSelection()
{
    ObjectIt _F = m_Objects.begin();
    while(_F!=m_Objects.end())
    {
        if((*_F)->Selected() && !(*_F)->m_CO_Flags.test(CCustomObject::flObjectInGroup))
        {
            if ((*_F)->OnSelectionRemove())
            {
                ObjectIt _D = _F; _F++;
                CCustomObject* obj 	= *_D; 
                Scene->RemoveObject	(obj,false,true);
                xr_delete			(obj);
            }else{
                _F++;
            }
        }else{
            _F++;
        }
    }
	UI->RedrawScene		();
}

void ESceneCustomOTool::InvertSelection()
{
    for(ObjectIt _F = m_Objects.begin();_F!=m_Objects.end();_F++)
        if((*_F)->Visible()){
            (*_F)->Select(-1);
        }
        
    UI->RedrawScene		();
}

int ESceneCustomOTool::SelectionCount(bool testflag)
{
	int count = 0;

    for(ObjectIt _F = m_Objects.begin();_F!=m_Objects.end();_F++)
        if((*_F)->Visible()	&& ((*_F)->Selected() == testflag)) count++;
        
    return count;
}

void ESceneCustomOTool::ShowObjects(bool flag, bool bAllowSelectionFlag, bool bSelFlag)
{
    for(ObjectIt _F = m_Objects.begin();_F!=m_Objects.end();_F++){
        if (bAllowSelectionFlag){
            if ((*_F)->Selected()==bSelFlag){
                (*_F)->Show( flag );
            }
        }else{
            (*_F)->Show( flag );
        }
    }
    UI->RedrawScene();
}

BOOL ESceneCustomOTool::RayPick(CCustomObject*& object, float& distance, const Fvector& start, const Fvector& direction, SRayPickInfo* pinf)
{
	object = 0;
    for(ObjectIt _F = m_Objects.begin();_F!=m_Objects.end();_F++)
        if((*_F)->Visible()&&(*_F)->RayPick(distance,start,direction,pinf))
            object=*_F;
	return !!object;
}

BOOL ESceneCustomOTool::FrustumPick(ObjectList& lst, const CFrustum& frustum)
{
    for(ObjectIt _F = m_Objects.begin();_F!=m_Objects.end();_F++)
        if((*_F)->Visible()&&(*_F)->FrustumPick(frustum))
        	lst.push_back(*_F);
	return !lst.empty();
}

BOOL ESceneCustomOTool::SpherePick(ObjectList& lst, const Fvector& center, float radius)
{
    for(ObjectIt _F = m_Objects.begin();_F!=m_Objects.end();_F++)
        if((*_F)->Visible()&&(*_F)->SpherePick(center, radius))
        	lst.push_back(*_F);
	return !lst.empty();
}

int ESceneCustomOTool::RaySelect(int flag, float& distance, const Fvector& start, const Fvector& direction, BOOL bDistanceOnly)
{
    CCustomObject* nearest_object=0;
    if (RayPick(nearest_object,distance,start,direction,0)&&!bDistanceOnly) 
    	nearest_object->RaySelect(flag,start,direction,false);
//    	nearest_object->Select(flag);
    UI->RedrawScene();
    return !!nearest_object;
}

int ESceneCustomOTool::FrustumSelect(int flag, const CFrustum& frustum)
{
	ObjectList lst;

    FrustumPick(lst,frustum);
    for(ObjectIt _F = lst.begin();_F!=lst.end();_F++)
	    (*_F)->Select(flag);
    
	return 0;
}

int ESceneCustomOTool::GetQueryObjects(ObjectList& lst, int iSel, int iVis, int iLock)
{
	int count=0;
    for(ObjectIt _F = m_Objects.begin();_F!=m_Objects.end();_F++)
    {
        if(	((iSel==-1)||((*_F)->Selected()==iSel))&&
            ((iVis==-1)||((*_F)->Visible()==iVis)) )
            {
                lst.push_back(*_F);
                count++;
        	}
    }
    return count;
}

CCustomObject* ESceneCustomOTool::FindObjectByName(LPCSTR name, CCustomObject* pass)
{
	ObjectIt _I = m_Objects.begin();
    ObjectIt _E = m_Objects.end();
	for(;_I!=_E;_I++) 
    {
    	CCustomObject* CO = (*_I);
    	LPCSTR _name = CO->Name;
        R_ASSERT	(_name);
    	if((pass!=*_I) && (0==strcmp(_name,name)) ) 
        	return (*_I);
    }
    return 0;
}

void setEditable(PropItemVec& items, u32 start_idx, bool bEditableTool, bool bObjectInGroup, bool bObjectInGroupUnique)
{
	PropItemIt it 	= items.begin()+start_idx;
	PropItemIt it_e = items.end();
	u32 idx=0;
    
    bool bEditableObject = bEditableTool && (!bObjectInGroup || (bObjectInGroup&&bObjectInGroupUnique) );
    
    for(; it!=it_e;++it,++idx)
    {
        bool bEnabledItem 	= bEditableObject;
        if(!bEnabledItem && idx==4 && bObjectInGroup)
            bEnabledItem = true;
            
    	(*it)->Enable(bEnabledItem);
    }
}

void ESceneCustomOTool::FillProp(LPCSTR pref, PropItemVec& items)
{
    for (ObjectIt it=m_Objects.begin(); it!=m_Objects.end(); it++)  
    {
        if ((*it)->Selected())
        {
        	
            u32 cnt = items.size();
            (*it)->FillProp	(PrepareKey(pref,"Items").c_str(), items);

           	setEditable				(	items, 
            							cnt,
            							IsEditable(), 
                                        (*it)->m_CO_Flags.test(CCustomObject::flObjectInGroup),
                                        (*it)->m_CO_Flags.test(CCustomObject::flObjectInGroupUnique));
        }
    }
}

/*
void ESceneCustomOTool::FillProp(LPCSTR pref, PropItemVec& items)
{
    for (ObjectIt it=m_Objects.begin(); it!=m_Objects.end(); ++it)  
    {
        if ((*it)->Selected())
            (*it)->FillProp	(PrepareKey(pref,"Items").c_str(), items);
    }
}
*/

bool ESceneCustomOTool::GetSummaryInfo(SSceneSummary* inf)
{
    for (ObjectIt it=m_Objects.begin(); it!=m_Objects.end(); it++)
    	(*it)->GetSummaryInfo(inf);
    return true;
}

void ESceneCustomOTool::GetBBox(Fbox& BB, bool bSelOnly)
{
	Fbox bb;
    ObjectList lst;
    if (GetQueryObjects(lst, bSelOnly, true, -1)){
        for(ObjectIt _F = lst.begin();_F!=lst.end();_F++)
            if ((*_F)->GetBox(bb)) BB.merge(bb);
    }
}

int ESceneCustomOTool::MultiRenameObjects()
{
	int cnt			= 0;
    for (ObjectIt o_it=m_Objects.begin(); o_it!=m_Objects.end(); o_it++){
    	CCustomObject* obj	= *o_it;
    	if (obj->Selected()){
            string256 			buf;
        	Scene->GenObjectName(obj->ClassID,buf,obj->RefName());
            if (obj->Name!=buf){
	            obj->Name		= buf;
                cnt++; 
            }
        }
    }
    return cnt;
}

void ESceneCustomOTool::OnSelected(CCustomObject* object)
{
    for (ObjectIt o_it=m_Objects.begin(); o_it!=m_Objects.end(); ++o_it)
    {
    	CCustomObject* obj	= *o_it;
    	obj->m_RT_Flags.set(CCustomObject::flRT_SelectedLast, FALSE);
	}
    object->m_RT_Flags.set(CCustomObject::flRT_SelectedLast, TRUE);
}

const CCustomObject* ESceneCustomOTool::LastSelected() const
{
    for (ObjectList::const_iterator o_it=m_Objects.begin(); o_it!=m_Objects.end(); ++o_it)
    {
    	const CCustomObject* obj	= *o_it;
    	if(obj->m_RT_Flags.test(CCustomObject::flRT_SelectedLast))
        	return obj;
	}
    return NULL;
}



