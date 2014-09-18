#include "stdafx.h"
#pragma hdrstop

#include "scene.h"
#include "leftbar.h"
#include "ui_leveltools.h"
#include "SceneObject.h"
#include "../ECore/Editor/UI_Main.h"
//------------------------------------------------------------------------------
 
ObjectList* EScene::GetSnapList(bool bIgnoreUse)
{
	ObjClassID cls 			= LTools->CurrentClassID();
    ESceneToolBase* mt 		= m_SceneTools[cls];
    if (0==mt)				return 0;
    ObjectList* snap_list	= mt->GetSnapList()?mt->GetSnapList():&m_ESO_SnapObjects;
    return bIgnoreUse?snap_list:(fraLeftBar->ebUseSnapList->Down?snap_list:NULL);
}
//--------------------------------------------------------------------------------------------------

bool EScene::FindObjectInSnapList(CCustomObject* O)
{
    ObjectList* snap_objects = GetSnapList(true);
	return snap_objects?std::find(snap_objects->begin(),snap_objects->end(),O)!=snap_objects->end():false;
}

void EScene::RenderSnapList()
{
    ObjectList* lst = GetSnapList(false);
    if (lst){
        for(ObjectIt _F=lst->begin();_F!=lst->end();_F++) 
            if((*_F)->Visible()) ((CSceneObject*)(*_F))->RenderSelection();
    }
}
//------------------------------------------------------------------------------

bool EScene::AddToSnapList(CCustomObject* O, bool bUpdateScene)
{
	if (!O) return false;
    ObjectList* snap_objects = GetSnapList(true);
    if (snap_objects){
        if (std::find(snap_objects->begin(),snap_objects->end(),O)==snap_objects->end()){
            snap_objects->push_back(O);
            if (bUpdateScene){
	            UI->RedrawScene();
    	        UpdateSnapList();
	 			UndoSave();
            }
		    return true;
        }
    }
    return false;
}
//------------------------------------------------------------------------------

bool EScene::DelFromSnapList(CCustomObject* O, bool bUpdateScene)
{
    ObjectList* snap_objects = GetSnapList(true);
    if (snap_objects){
        ObjectIt it = std::find(snap_objects->begin(),snap_objects->end(),O);
        if (it!=snap_objects->end()){
            snap_objects->erase(it);
            if (bUpdateScene){
	            UI->RedrawScene();
    	        UpdateSnapList();
	 			UndoSave();
            }
            return true;
        }
    }
    return false;
}
//------------------------------------------------------------------------------

int EScene::AddSelToSnapList()
{
	int count = 0;
    if (GetSnapList(true)){
        ObjectList lst;
        int count = GetQueryObjects(lst, OBJCLASS_SCENEOBJECT, 1, 1, 0);
        if (count){
            count = 0;
            for(ObjectIt _F = lst.begin();_F!=lst.end();_F++)
            	if (AddToSnapList(*_F,false)) count++;
            if (count){
                UI->RedrawScene();
                UpdateSnapList();
	 			UndoSave();
            }
        }
    }
	return count;
}
//------------------------------------------------------------------------------

int EScene::DelSelFromSnapList()
{
	int count = 0;
    if (GetSnapList(true)){
        ObjectList lst;
        int count = GetQueryObjects(lst, OBJCLASS_SCENEOBJECT, 1, 1, 0);
        if (count){
            count = 0;
            for(ObjectIt _F = lst.begin();_F!=lst.end();_F++)
            	if (DelFromSnapList(*_F,false)) count++;
            if (count){
                UI->RedrawScene();
                UpdateSnapList();
	 			UndoSave();
            }
        }
    }
    return count;
}
//------------------------------------------------------------------------------

int EScene::SetSnapList()
{
	ClearSnapList(true);
    ObjectList* snap_objects = GetSnapList(true);
	int count = 0;
    if (snap_objects){
        ObjectList& lst = ListObj(OBJCLASS_SCENEOBJECT);
        for(ObjectIt _F = lst.begin();_F!=lst.end();_F++)
            if((*_F)->Visible()&&(*_F)->Selected()){
                snap_objects->push_back(*_F);
                count++;
            }
        UI->RedrawScene();
        UpdateSnapList();
		UndoSave();
    }
	return count;
}
//------------------------------------------------------------------------------

void EScene::ClearSnapList(bool bCurrentOnly)
{
	if (bCurrentOnly){
	    ObjectList* snap_objects = GetSnapList(true);
    	if (snap_objects){
	        snap_objects->clear();
    	    UpdateSnapList();
            UndoSave();
    	}
    }else{
    	m_ESO_SnapObjects.clear();
        for (int i=0; i<OBJCLASS_COUNT; i++){
	        ESceneToolBase* mt 		= m_SceneTools[ObjClassID(i)];
            if (mt&&mt->GetSnapList())	mt->GetSnapList()->clear();
        }
        UpdateSnapList();
    }
}
//------------------------------------------------------------------------------

void EScene::SelectSnapList()
{
    ObjectList* snap_objects = GetSnapList(true);
    if (snap_objects){
        SelectObjects(FALSE,OBJCLASS_SCENEOBJECT);
        for(ObjectIt _F = snap_objects->begin();_F!=snap_objects->end();_F++)
            (*_F)->Select(TRUE);
        UI->RedrawScene();
    }
}
//------------------------------------------------------------------------------

void EScene::UpdateSnapList()
{
    m_RTFlags.set(flUpdateSnapList, TRUE);
}

void EScene::UpdateSnapListReal()
{
	if (NULL==LTools) return;
    ObjClassID cls = LTools->CurrentClassID();
    switch (cls){
    case OBJCLASS_DUMMY:	break;
    default:
	    ESceneToolBase* mt = m_SceneTools[cls];
    	if (mt) mt->UpdateSnapList();
    }
	// visual update
	if (fraLeftBar) fraLeftBar->UpdateSnapList();

    m_RTFlags.set(flUpdateSnapList, FALSE);
}
//------------------------------------------------------------------------------
