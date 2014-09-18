#include "stdafx.h"
#pragma hdrstop

#include "ESceneSectorTools.h"
#include "ESceneSectorControls.h"
#include "ui_leveltools.h"
#include "FrameSector.h"
#include "SceneObject.h"
#include "GroupObject.h"
#include "sector.h"

/* TODO 1 -oAlexMX -cTODO: Create tools as AI Map */

void ESceneSectorTool::CreateControls()
{
//	inherited::CreateControls();
    AddControl		(xr_new<TUI_ControlSectorSelect>(estDefault,etaSelect,	this));
    AddControl		(xr_new<TUI_ControlSectorAdd>	(estDefault,etaAdd,		this));
	// frame
    pFrame 			= xr_new<TfraSector>((TComponent*)0);
}
//----------------------------------------------------

void ESceneSectorTool::RemoveControls()
{
	inherited::RemoveControls();
}
//----------------------------------------------------
void ESceneSectorTool::_OnObjectRemove(CSceneObject* obj)
{
    if (obj && !m_Objects.empty())
    {
        EditMeshVec* meshes 		= obj->Meshes();
        for (EditMeshIt m_it= meshes->begin(); m_it!=meshes->end(); ++m_it)
        {
            for(ObjectIt _F=m_Objects.begin();_F!=m_Objects.end();++_F)
            {
                CSector* sector 	= dynamic_cast<CSector*>(*_F);
                VERIFY				(sector);
                if (sector->DelMesh(obj, *m_it))
                    break;
            }
        }
    }
}

void ESceneSectorTool::OnObjectRemove(CCustomObject* O, bool bDeleting)
{
	inherited::OnObjectRemove(O, bDeleting);

    if(bDeleting)
    {
        CSceneObject* obj = dynamic_cast<CSceneObject*>(O);
        if(!obj)
        {   /*
			CGroupObject* go = dynamic_cast<CGroupObject*>(O);
            if(go && !go->IsOpened())
            {
            	ObjectList 					lst;
                go->GetObjects				(lst);
                
                ObjectList::iterator it 	= lst.begin();
                ObjectList::iterator it_e 	= lst.end();
                for(; it!=it_e; ++it)
                {
        			CSceneObject* obj2 	= dynamic_cast<CSceneObject*>(*it);
					_OnObjectRemove(obj2);
                }
            } */
        }else
        	_OnObjectRemove(obj);
        
    }
}

void ESceneSectorTool::OnBeforeObjectChange(CCustomObject* O)
{
	inherited::OnBeforeObjectChange(O);

    CSceneObject* obj = dynamic_cast<CSceneObject*>(O);
    if (obj&&!m_Objects.empty()){
	    EditMeshVec* meshes = obj->Meshes();
        for (EditMeshIt m_it= meshes->begin(); m_it!=meshes->end(); m_it++){
	        for(ObjectIt _F=m_Objects.begin();_F!=m_Objects.end();_F++){
    	        CSector* sector = dynamic_cast<CSector*>(*_F); VERIFY(sector);
                if (sector->DelMesh(obj, *m_it)) break;
            }
        }
	}
}

//----------------------------------------------------

void ESceneSectorTool::FillProp(LPCSTR pref, PropItemVec& items)
{
	PHelper().CreateFlag32(items, PrepareKey(pref,"Common\\Draw Solid"),&m_Flags,			flDrawSolid);
	inherited::FillProp	(pref, items);
}
//----------------------------------------------------

CCustomObject* ESceneSectorTool::CreateObject(LPVOID data, LPCSTR name)
{
	CCustomObject* O	= xr_new<CSector>(data,name);
    O->ParentTool		= this;
    return O;
}
//----------------------------------------------------

