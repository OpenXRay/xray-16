#include "stdafx.h"
#pragma hdrstop

#include "GroupObject.h"
#include "Scene.h"
#include "ESceneCustomMTools.h"
//----------------------------------------------------
//----------------------------------------------------
#define GROUPOBJ_CURRENT_VERSION		0x0012
//----------------------------------------------------
#define GROUPOBJ_CHUNK_VERSION		  	0x0000
#define GROUPOBJ_CHUNK_OBJECT_LIST     	0x0001
#define GROUPOBJ_CHUNK_FLAGS	     	0x0003
#define GROUPOBJ_CHUNK_REFERENCE	  	0x0004
#define GROUPOBJ_CHUNK_OPEN_OBJECT_LIST	0x0005

CGroupObject::CGroupObject(LPVOID data, LPCSTR name):CCustomObject(data,name)
{
	Construct	(data);
}

void CGroupObject::Construct(LPVOID data)
{
	ClassID		= OBJCLASS_GROUP;
}

CGroupObject::~CGroupObject()
{
}

u32 CGroupObject::GetObjects(ObjectList& lst)
{
	lst.clear();

    for (ObjectsInGroup::iterator it=m_ObjectsInGroup.begin(); it!=m_ObjectsInGroup.end(); ++it)
        lst.push_back	(it->pObject);

    return lst.size();
}

void CGroupObject::Select(int  flag)
{
    inherited::Select(flag);
    if(EPrefs->object_flags.test(epoSelectInGroup))
    {
    for (ObjectsInGroup::iterator it=m_ObjectsInGroup.begin(); it!=m_ObjectsInGroup.end(); ++it)
        it->pObject->Select(flag);
    }
}

void CGroupObject::ClearInternal(ObjectsInGroup& v)
{
    for (ObjectsInGroup::iterator it=v.begin(); it!=v.end(); ++it)
    {
        it->pObject->m_pOwnerObject		= 0;
        Scene->RemoveObject				(it->pObject, false, true);
        xr_delete						(it->pObject);
    }
    v.clear();
}


void CGroupObject::OnFrame()
{
	inherited::OnFrame();
	for (ObjectsInGroup::iterator it=m_ObjectsInGroup.begin(); it!=m_ObjectsInGroup.end(); ++it)
    	it->pObject->OnFrame();
}

bool CGroupObject::LL_AppendObject(CCustomObject* object)
{
    if (!object->CanAttach())
    {
    	ELog.Msg(mtError,"Can't attach object: '%s'",object->Name);
	    return false;
    }
    if(object->GetOwner())
    {
        if (mrNo==ELog.DlgMsg(mtConfirmation,TMsgDlgButtons() << mbYes << mbNo,"Object '%s' already in group '%s'. Change group?",object->Name,object->GetOwner()->Name))
        	return false;
            
	    object->OnDetach();
    }

	object->OnAttach		(this);
    object->m_CO_Flags.set	(flObjectInGroupUnique, FALSE);
    m_ObjectsInGroup.resize	(m_ObjectsInGroup.size()+1);
    m_ObjectsInGroup.back().pObject = object;

    return true;
}

bool CGroupObject::AppendObjectLoadCB(CCustomObject* object)
{
    object->m_pOwnerObject			= this;
    object->m_CO_Flags.set			(flObjectInGroup, TRUE);
    m_ObjectsInGroup.resize			(m_ObjectsInGroup.size()+1);
    m_ObjectsInGroup.back().pObject = object;


    string256 			buf;
    Scene->GenObjectName(object->ClassID, buf, object->Name);
    LPCSTR N = object->Name;
    if (xr_strcmp(N,buf)!=0)
    {
       	Msg					("Load_Append name changed from[%s] to[%s]",object->Name, buf);
        object->Name		= buf;
    }

	Scene->AppendObject				(object, false);
    
    return true;
}

void  CGroupObject::SetRefName(LPCSTR nm)
{
	m_ReferenceName_ = nm;
    if(nm && strstr(nm, "light_preset"))
    	save_id = 1;
    else
    	save_id = 0;
}

#include "AppendObjectInfoForm.h"

//------------------------------------------------------------------------------
bool CGroupObject::LoadLTX(CInifile& ini, LPCSTR sect_name)
{
    u32 version = ini.r_u32(sect_name, "version");
    if (version<0x0011)
    {
        ELog.DlgMsg( mtError, "CGroupObject: unsupported file version. Object can't load.");
        return false;
    }
	CCustomObject::LoadLTX(ini, sect_name);

    Flags32 tmp_flags;tmp_flags.zero();
    if(version<0x0012)
    	tmp_flags.assign(ini.r_u32(sect_name, "flags") );

	// objects
    if(/*IsOpened()*/ tmp_flags.test((1<<0)))
    {    //old opened group save format
        ELog.DlgMsg( mtError, "old opened group save format");
        return false;
/*      
        u32 cnt 	= ini.r_u32			(sect_name, "objects_in_group_count");
        shared_str 		tmp;
    	string128		buff;

        for (u32 k=0; k<cnt; ++k)
        {
			m_ObjectsInGroup.resize				(m_ObjectsInGroup.size()+1);
        	sprintf								(buff,"objects_in_group_%d",k);
        	m_ObjectsInGroup.back().ObjectName	= ini.r_string(sect_name, buff);
        }
*/        
    }else
    {
	    Scene->ReadObjectsLTX			(ini, sect_name, "ingroup", AppendObjectLoadCB, 0);
    }
    VERIFY(m_ObjectsInGroup.size());

   	SetRefName(ini.r_string	(sect_name, "ref_name")) ;
	if (!m_ReferenceName_.size())
        ELog.Msg			(mtError,"ERROR: group '%s' - has empty reference. Corrupted file?", Name);
    

    if(version<0x0012)
    {
        for (ObjectsInGroup::iterator it=m_ObjectsInGroup.begin(); it!=m_ObjectsInGroup.end(); ++it)
            if(it->pObject)
            {
            	it->pObject->m_CO_Flags.set(flObjectInGroup, TRUE);
            	it->pObject->m_CO_Flags.set(flObjectInGroupUnique, TRUE);
            }
    }

    return 			true;
}

void CGroupObject::SaveLTX(CInifile& ini, LPCSTR sect_name)
{
	CCustomObject::SaveLTX(ini, sect_name);

	ini.w_u32			(sect_name, "version", GROUPOBJ_CURRENT_VERSION);

    ObjectList		grp_lst;
    GetObjects		(grp_lst);
    ObjectList::iterator it;
    for(it=grp_lst.begin(); it!=grp_lst.end(); ++it)
        (*it)->m_CO_Flags.set(CCustomObject::flObjectInGroup, FALSE);
        
    Scene->SaveObjectsLTX(grp_lst, sect_name, "ingroup", ini);

    for(it=grp_lst.begin(); it!=grp_lst.end(); ++it)
        (*it)->m_CO_Flags.set(CCustomObject::flObjectInGroup, TRUE);

    ini.w_string		(sect_name, "ref_name", m_ReferenceName_.c_str());
}

bool CGroupObject::LoadStream(IReader& F)
{
    u16 version=0;

    R_ASSERT(F.r_chunk(GROUPOBJ_CHUNK_VERSION,&version));
    if (version<0x0011)
    {
        ELog.DlgMsg( mtError, "CGroupObject: unsupported file version. Object can't load.");
        return false;
    }
	CCustomObject::LoadStream(F);

    Flags32 tmp_flags; tmp_flags.zero();
    if(version<0x0012)
    	F.r_chunk(GROUPOBJ_CHUNK_FLAGS,&tmp_flags);

	// objects
    if (tmp_flags.test(1<<0))
    { //old format, opened group
        ELog.DlgMsg( mtError, "old format, opened group");
        return false;
/*        
        R_ASSERT(F.find_chunk(GROUPOBJ_CHUNK_OPEN_OBJECT_LIST));
        u32 cnt 	= F.r_u32();
        for (u32 k=0; k<cnt; ++k)
        {
			m_ObjectsInGroup.resize	(m_ObjectsInGroup.size()+1);
            F.r_stringZ				(m_ObjectsInGroup.back().ObjectName);
        }
*/        
    }else
    {
	    Scene->ReadObjectsStream(F,GROUPOBJ_CHUNK_OBJECT_LIST,AppendObjectLoadCB,0);
    }
    VERIFY(m_ObjectsInGroup.size());

    if (F.find_chunk(GROUPOBJ_CHUNK_REFERENCE))	
    {
    	shared_str rn;
    	F.r_stringZ	(rn);
        SetRefName(rn.c_str());
     }
     
    if(version<0x0012)
    {
        for (ObjectsInGroup::iterator it=m_ObjectsInGroup.begin(); it!=m_ObjectsInGroup.end(); ++it)
            if(it->pObject)
            {
            	it->pObject->m_CO_Flags.set(flObjectInGroup, TRUE);
            	it->pObject->m_CO_Flags.set(flObjectInGroupUnique, TRUE);
            }
    }

    return 			true;
}

void CGroupObject::SaveStream(IWriter& F)
{
	CCustomObject::SaveStream(F);

	F.open_chunk	(GROUPOBJ_CHUNK_VERSION);
	F.w_u16			(GROUPOBJ_CURRENT_VERSION);
	F.close_chunk	();

    {
    	ObjectList		grp_lst;
    	GetObjects		(grp_lst);
        ObjectList::iterator it;
        for (it=grp_lst.begin(); it!=grp_lst.end(); ++it)
            (*it)->m_CO_Flags.set(CCustomObject::flObjectInGroup, FALSE);

	    Scene->SaveObjectsStream(grp_lst,GROUPOBJ_CHUNK_OBJECT_LIST,F);

        for (it=grp_lst.begin(); it!=grp_lst.end(); ++it)
            (*it)->m_CO_Flags.set(CCustomObject::flObjectInGroup, TRUE);
    }

    F.open_chunk	(GROUPOBJ_CHUNK_REFERENCE);
    F.w_stringZ		(m_ReferenceName_);
	F.close_chunk	();
}
//----------------------------------------------------

bool CGroupObject::ExportGame(SExportStreams* data)
{
	return true;
}

void CGroupObject::ReferenceChange(PropValue* sender)
{
	UpdateReference		(true);
}
//----------------------------------------------------

bool CGroupObject::SetReference(LPCSTR ref_name)
{
	shared_str old_refs	= m_ReferenceName_;
    SetRefName(ref_name);
    bool bres 			= UpdateReference(old_refs!=ref_name);
	if (false==bres)	
    	SetRefName(old_refs.c_str());
        
    return bres;
}
#include "../ECore/Editor/ui_main.h"
#include "UI_LevelMain.h"

extern BOOL bForceInitListBox;

bool CGroupObject::UpdateReference(bool bForceReload)
{
	if (!m_ReferenceName_.size())
    {
        ELog.Msg			(mtError,"ERROR: '%s' - has empty reference.",Name);
     	return 				false;
    }

    
    xr_string fn			= m_ReferenceName_.c_str();
    fn						= EFS.ChangeFileExt(fn,".group");
    IReader* R				= FS.r_open(_groups_, fn.c_str());
    bool bres				= false;
    if (R)
    {
    	bForceInitListBox	= TRUE;
  	  	ObjectsInGroup		ObjectsInGroupBk = m_ObjectsInGroup;

    	if(bForceReload)
          ClearInternal		(ObjectsInGroupBk);
        
        m_ObjectsInGroup.clear();
        

        xr_string nm		= Name;
		shared_str old_refs	= m_ReferenceName_;
        UpdateTransform		(true);
        Fvector old_pos		= PPosition;
        Fvector old_rot		= PRotation;
        Fvector old_sc		= PScale;

        if(LoadStream(*R))
        {
            Name 		= nm.c_str();
            bres		= true;
	        UpdateTransform	(true);
        }
        FS.r_close			(R);
	
    	SetRefName(old_refs.c_str());
        NumSetPosition		(old_pos);
        NumSetRotation		(old_rot);
        NumSetScale			(old_sc);
        UpdateTransform		(true);

		if(m_ObjectsInGroup.size()==ObjectsInGroupBk.size())
        {
        	ObjectsInGroup::iterator it = m_ObjectsInGroup.begin();
        	ObjectsInGroup::iterator itBk = ObjectsInGroupBk.begin();
            for(; it!=m_ObjectsInGroup.end(); ++it, ++itBk)
            {
				it->pObject->m_CO_Flags.set(flObjectInGroupUnique, FALSE);
               
               if(itBk->pObject->m_CO_Flags.test(flObjectInGroupUnique) )
               {
               	  std::swap(*itBk, *it);
               }else
               	 it->pObject->Name = itBk->pObject->Name;
            }
        }else
        {
        	if(ObjectsInGroupBk.size())
            {
        		ELog.Msg		(mtError, "Not all objects synchronised correctly", Name);
                for (ObjectsInGroup::iterator it=m_ObjectsInGroup.begin(); it!=m_ObjectsInGroup.end(); ++it)
                        it->pObject->m_CO_Flags.set(flObjectInGroupUnique, FALSE);
            }else
            { // first setup
                for (ObjectsInGroup::iterator it=m_ObjectsInGroup.begin(); it!=m_ObjectsInGroup.end(); ++it)
                        it->pObject->m_CO_Flags.set(flObjectInGroupUnique, FALSE);
            }
        }

        ClearInternal		(ObjectsInGroupBk);
        
    }else
    {
        ELog.Msg		(mtError,"ERROR: Can't open group file: '%s'.",fn.c_str());
    }
    
    return bres;
}
//----------------------------------------------------

void CGroupObject::FillProp(LPCSTR pref, PropItemVec& items)
{
	inherited::FillProp(pref, items);
    PropValue* V		= PHelper().CreateChoose	(items,PrepareKey(pref,"Reference"),&m_ReferenceName_, smGroup); 
    V->OnChangeEvent.bind(this,&CGroupObject::ReferenceChange);

    ButtonValue* B;
    B=PHelper().CreateButton	(items,PrepareKey(pref,"FreezeObjects"), 	"Unique,Reference",0);
    B->OnBtnClickEvent.bind		(this,&CGroupObject::OnFreezeAllClick);

	for (ObjectsInGroup::iterator it=m_ObjectsInGroup.begin(); it!=m_ObjectsInGroup.end(); ++it)
    {
    	string_path					pk;
        sprintf						(pk,"%s (%s)",PrepareKey(pref, it->pObject->Name).c_str(),it->pObject->ParentTool->ClassDesc());
        V = PHelper().CreateFlag32	(items, pk, &it->pObject->m_CO_Flags, flObjectInGroupUnique);
    	V->OnChangeEvent.bind		(this,&CCustomObject::OnChangeIngroupUnique);
    }
}

void CGroupObject::OnFreezeAllClick(ButtonValue* sender, bool& bModif, bool& bSafe)
{
	ButtonValue* V = dynamic_cast<ButtonValue*>(sender);
    BOOL bDoUnique = FALSE;
    switch (V->btn_num)
    {
        case 0: 
        {   
         	bDoUnique = TRUE;
        }break;
        case 1:
        {   
         	bDoUnique = FALSE;
        }break;
	}
	for (ObjectsInGroup::iterator it=m_ObjectsInGroup.begin(); it!=m_ObjectsInGroup.end(); ++it)
    	it->pObject->m_CO_Flags.set(flObjectInGroupUnique, bDoUnique);
    
    bModif = true;
}

//----------------------------------------------------

void CGroupObject::OnShowHint(AStringVec& dest)
{
	inherited::OnShowHint(dest);
    dest.push_back(AnsiString("Reference: ")+m_ReferenceName_.c_str());
    dest.push_back(AnsiString("-------"));
	for (ObjectsInGroup::iterator it=m_ObjectsInGroup.begin(); it!=m_ObjectsInGroup.end(); ++it)
	    dest.push_back(it->pObject->Name);
}
//----------------------------------------------------

void CGroupObject::OnObjectRemove(const CCustomObject* object)
{
    ObjectsInGroup::iterator it = std::find(m_ObjectsInGroup.begin(),m_ObjectsInGroup.end(),object);
    if(it==m_ObjectsInGroup.end())
        return;
            
    m_ObjectsInGroup.erase(it);
}

void CGroupObject::OnSceneUpdate()
{
	inherited::OnSceneUpdate();

    	for (ObjectsInGroup::iterator it=m_ObjectsInGroup.begin(); it!=m_ObjectsInGroup.end(); ++it)
        {
        	if(it->pObject==NULL)
            {
            	R_ASSERT(0);
/*            
            	R_ASSERT(it->ObjectName.size());
                CCustomObject* CO 	= Scene->FindObjectByName(it->ObjectName.c_str(), (CCustomObject*)0);
               
                R_ASSERT2			(!CO->m_CO_Flags.test(flObjectInGroup), it->ObjectName.c_str());
                it->pObject			= CO;

                string256 			buf;
                Scene->GenObjectName(CO->ClassID, buf, CO->Name);
                if (CO->Name!=buf)
                {
                	Msg("OG name changed from[%s] to[%s]",CO->Name, buf);
                    CO->Name		= buf;
					it->ObjectName	= buf;
                }
                
                it->pObject->m_CO_Flags.set(flObjectInGroup, 		TRUE);
                it->pObject->m_CO_Flags.set(flObjectInGroupUnique, 	TRUE);
                if(it->pObject==NULL)
                    ELog.Msg	(mtError,"Gr%s' has invalid roup 'eference to object '%s'.", Name, it->ObjectName.c_str());
*/                    
            }
            
    }
    if (m_ObjectsInGroup.empty())
    {
        ELog.Msg	(mtInformation,"Group '%s' has no objects and will be removed.",Name);
        DeleteThis	();
    }
}

//----------------------------------------------------
bool CGroupObject::CanUngroup(bool bMsg)
{
	bool res	= true;
	for (ObjectsInGroup::iterator it =m_ObjectsInGroup.begin(); it!=m_ObjectsInGroup.end(); ++it)
    {
    	ESceneToolBase* tool = Scene->GetTool(it->pObject->ClassID);
    	if ( !tool->IsEditable()  )
        {
        	if (bMsg) Msg("!.Can't detach object '%s'. Target '%s' in readonly mode.",it->pObject->Name, tool->ClassDesc());
        	res = false;
        }
    }
    return res;
}

void CGroupObject::GroupObjects(ObjectList& lst)
{
	R_ASSERT(lst.size());
	for (ObjectIt it=lst.begin(); it!=lst.end(); ++it)
        {
            if((*it)->m_CO_Flags.test(flObjectInGroup) )
            	{
        			ELog.DlgMsg	(mtInformation,"object[%s] already in group", (*it)->Name);
                    continue;
                }
    		LL_AppendObject(*it);
        }
	if(m_ObjectsInGroup.size())
	    UpdatePivot(0,false);
}

void CGroupObject::UngroupObjects()
{
    for (ObjectsInGroup::iterator it=m_ObjectsInGroup.begin(); it!=m_ObjectsInGroup.end(); ++it)
        it->pObject->OnDetach		();

    m_ObjectsInGroup.clear();
}

