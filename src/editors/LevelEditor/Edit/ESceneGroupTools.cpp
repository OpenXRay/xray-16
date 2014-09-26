#include "stdafx.h"
#pragma hdrstop

#include "ESceneGroupTools.h"
#include "ESceneGroupControls.h"
#include "ui_leveltools.h"
#include "FrameGroup.h"
#include "Scene.h"
#include "GroupObject.h"
#include "../ECore/Editor/EThumbnail.h"

void ESceneGroupTool::CreateControls()
{
	inherited::CreateDefaultControls(estDefault);
    AddControl		(xr_new<TUI_ControlGroupAdd >(estDefault,etaAdd,		this));
	// frame
    pFrame 			= xr_new<TfraGroup>((TComponent*)0,this);
}
//----------------------------------------------------

void ESceneGroupTool::RemoveControls()
{
	inherited::RemoveControls();
}
//----------------------------------------------------

void ESceneGroupTool::UngroupObjects(bool bUndo)
{
    ObjectList lst 	= m_Objects;
    int sel_cnt		= 0;
    if (!lst.empty())
    {
    	bool bModif	= false;
        for (ObjectIt it=lst.begin(); it!=lst.end(); ++it)
        {
            if ((*it)->Selected())
            {
            	sel_cnt++;
            	CGroupObject* obj 	= dynamic_cast<CGroupObject*>(*it); 
                VERIFY(obj);
                if (obj->CanUngroup(true))
                {
                    obj->UngroupObjects	();
                    Scene->RemoveObject	(obj,false,true);
                    xr_delete			(obj);
                    bModif				= true;
                }else
                    ELog.DlgMsg			(mtError,"Can't ungroup object: '%s'.",obj->Name);
            }
        }
        if (bUndo&&bModif) 
            Scene->UndoSave();
    }
    if (0==sel_cnt)
        ELog.Msg		(mtError,"Nothing selected.");
}
//----------------------------------------------------

BOOL  ESceneGroupTool::_RemoveObject(CCustomObject* object)
{
	inherited::_RemoveObject(object);

    CGroupObject* go 	= dynamic_cast<CGroupObject*>(object); 
    go->Clear1          ();
    return              TRUE;
}

void ESceneGroupTool::GroupObjects(bool bUndo)
{
    string256                   namebuffer;
    Scene->GenObjectName        (OBJCLASS_GROUP, namebuffer);
    CGroupObject* group         = xr_new<CGroupObject>((LPVOID)0, namebuffer);

    // validate objects
    ObjectList lst;
    if (Scene->GetQueryObjects(lst,OBJCLASS_DUMMY,1,1,0)) 
    	group->GroupObjects(lst);
        
    if (group->ObjectInGroupCount())
    {
		ELog.DlgMsg(mtInformation,"Group '%s' successfully created.\nContain %d object(s)",group->Name,group->ObjectInGroupCount());
        Scene->AppendObject(group, bUndo);
    }else
	{
		ELog.DlgMsg	(mtError,"Group can't be created.");
        xr_delete	(group);
    }
}

void ESceneGroupTool::CenterToGroup()
{
    ObjectList& lst 	= m_Objects;
    if (!lst.empty())
    {
    	for (ObjectIt it=lst.begin(); it!=lst.end(); ++it)
        	((CGroupObject*)(*it))->UpdatePivot(0, true);

    	Scene->UndoSave();
    }
}
//----------------------------------------------------

void __stdcall  FillGroupItems(ChooseItemVec& items, void* param)
{
	CGroupObject* group = (CGroupObject*)param;
    ObjectList 			grp_lst;
    group->GetObjects	(grp_lst);
    
    for (ObjectIt it=grp_lst.begin(); it!=grp_lst.end(); ++it)
	    items.push_back	(SChooseItem((*it)->Name,""));
}

void ESceneGroupTool::AlignToObject()
{
    ObjectList& lst 	= m_Objects;
    int sel_cnt			= 0;
    if (!lst.empty())
	{
        LPCSTR nm;
    	for (ObjectIt it=lst.begin(); it!=lst.end(); ++it)
		{
        	if ((*it)->Selected())
			{
			    sel_cnt++;
                if (TfrmChoseItem::SelectItem(smCustom, nm, 1, nm, FillGroupItems, *it))
				{
                    ((CGroupObject*)(*it))->UpdatePivot(nm,false);
                }else 
					break;
            }
        }
        Scene->UndoSave();
    }
    if (0==sel_cnt)	
		ELog.Msg(mtError,"Nothing selected.");
}
//----------------------------------------------------

CCustomObject* ESceneGroupTool::CreateObject(LPVOID data, LPCSTR name)
{
	CCustomObject* O	= xr_new<CGroupObject>(data, name);
    O->ParentTool		= this;
    return O;
}

void ESceneGroupTool::ReloadRefsSelectedObject()
{
    ObjectList lst 				= m_Objects;
    int sel_cnt					= 0;
    if (!lst.empty())
    {
        string_path				temp_file_name_sector,temp_file_name_portal;
        GetTempFileName			( FS.get_path(_temp_)->m_Path, "tmp_sector", 0, temp_file_name_sector );
        Scene->SaveToolLTX		(OBJCLASS_SECTOR, temp_file_name_sector);

        GetTempFileName			( FS.get_path(_temp_)->m_Path, "tmp_portal", 0, temp_file_name_portal );
        Scene->SaveToolLTX		(OBJCLASS_PORTAL, temp_file_name_portal);

   		bool bModif	= false;
        for (ObjectIt it=lst.begin(); it!=lst.end(); ++it)
		{
        	if ((*it)->Selected())
            {
			    sel_cnt++;
            	CGroupObject* obj 	= dynamic_cast<CGroupObject*>(*it); 
                VERIFY				(obj);
                if (obj->UpdateReference(true))
                {
                    bModif		= true;
                }else
                {
                    ELog.Msg	(mtError,"Can't reload group: '%s'.",obj->Name);
                }
            }
        }
	    if(bModif) 
        	Scene->UndoSave		();

		Scene->LoadToolLTX		(OBJCLASS_SECTOR, temp_file_name_sector);
		Scene->LoadToolLTX		(OBJCLASS_PORTAL, temp_file_name_portal);
    }
    if (0==sel_cnt)	
    	ELog.Msg	(mtError,"Nothing selected.");
}
//----------------------------------------------------

void ESceneGroupTool::SaveSelectedObject()
{
	u32 scnt = SelectionCount(true);
	if(scnt==0)
    {
        ELog.DlgMsg(mtError,"No object(s) selected.");
        return;
    }else
	if(scnt>1)
    {
        if(mrYes != ELog.DlgMsg(mtConfirmation, TMsgDlgButtons() << mbYes << mbNo, "Process multiple objects?") )
        	return;
    }
    
	CGroupObject* obj	= 0;
	// find single selected object
    for(ObjectIt it=m_Objects.begin(); it!=m_Objects.end(); ++it)
    {
    	if((*it)->Selected())
        {
        	obj 		= dynamic_cast<CGroupObject*>(*it);
            
            xr_string fn;
            if(scnt==1)
            {
            	fn 					= obj->RefName();
                if( !EFS.GetSaveName(_groups_,fn) )
                    return;
            }else
            {
            	string_path 		S;
               FS.update_path		(S, _groups_, obj->RefName());
               fn 					= S;
               fn 					+= ".group";
            }
            
            IWriter* W 				= FS.w_open(fn.c_str());
            if (W)
            {
                obj->SaveStream		(*W);
                FS.w_close			(W);
            }else
                ELog.DlgMsg			(mtError, "Cant write file [%s]", fn.c_str());
      }          
    }
}
//----------------------------------------------------

void ESceneGroupTool::SetCurrentObject(LPCSTR nm)
{
	m_CurrentObject				= nm;
	TfraGroup* frame			=(TfraGroup*)pFrame;
    frame->lbCurrent->Caption 	= m_CurrentObject.c_str();
}
//----------------------------------------------------

void ESceneGroupTool::OnActivate()
{
	inherited::OnActivate		();
	TfraGroup* frame			= (TfraGroup*)pFrame;
    frame->lbCurrent->Caption 	= m_CurrentObject.c_str();
}
//----------------------------------------------------

void ESceneGroupTool::MakeThumbnail()
{
	if (SelectionCount(true)==1)
    {
	    CGroupObject* object		= 0;
        for (ObjectIt it=m_Objects.begin(); it!=m_Objects.end(); it++)
        {
        	if ((*it)->Selected())
            {
	            object				= dynamic_cast<CGroupObject*>(*it);
                break;
            }
        }
        VERIFY						(object);
        object->Select				(false);
        // save render params
        Flags32 old_flag			= psDeviceFlags;
        // set render params
        psDeviceFlags.set			(rsStatistic|rsDrawGrid,FALSE);

        U32Vec pixels;
        u32 w=512,h=512;
        if (EDevice.MakeScreenshot	(pixels,w,h))
        {
            AnsiString tex_name		= ChangeFileExt(object->Name,".thm");
            SStringVec lst;

            ObjectList 				grp_lst;
            object->GetObjects		(grp_lst);
            
            for (ObjectIt it=grp_lst.begin(); it!=grp_lst.end(); ++it)
                lst.push_back		((*it)->Name);
                
            EGroupThumbnail 		tex	(tex_name.c_str(),false);
            tex.CreateFromData		(pixels.begin(),w,h,lst);
            string_path fn;
            FS.update_path			(fn,_groups_,object->RefName());
            strcat					(fn,".group");
            tex.Save				(FS.get_file_age(fn));
        }else
        {
            ELog.DlgMsg				(mtError,"Can't make screenshot.");
        }
        object->Select				(true);
        // restore render params
        psDeviceFlags 				= old_flag;
    }else
    {
    	ELog.DlgMsg		(mtError,"Select 1 GroupObject.");
    }
}
