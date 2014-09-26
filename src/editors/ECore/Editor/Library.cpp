//----------------------------------------------------
// file: Library.cpp
//----------------------------------------------------

#include "stdafx.h"
#pragma hdrstop

#include "Log.h"
#include "Library.h"
#include "EditObject.h"
#include "ui_main.h"

//----------------------------------------------------
ELibrary Lib;
//----------------------------------------------------
ELibrary::ELibrary()
{
    m_bReady  = false;
}
//----------------------------------------------------

ELibrary::~ELibrary()
{
}
//----------------------------------------------------

void ELibrary::OnCreate()
{
//	EDevice.seqDevCreate.Add	(this,REG_PRIORITY_NORMAL);
//	EDevice.seqDevDestroy.Add(this,REG_PRIORITY_NORMAL);
    m_bReady = true;
}
//----------------------------------------------------

void ELibrary::OnDestroy()
{
	VERIFY(m_bReady);
    m_bReady = false;
//	EDevice.seqDevCreate.Remove(this);
//	EDevice.seqDevDestroy.Remove(this);

    // remove all instance CEditableObject
	EditObjPairIt O = m_EditObjects.begin();
	EditObjPairIt E = m_EditObjects.end();
    for(; O!=E; O++){
    	if (0!=O->second->m_RefCount){
//.        	ELog.DlgMsg(mtError,"Object '%s' still referenced.",O->first.c_str());
//.	    	R_ASSERT(0==O->second->m_RefCount);
        }
    	xr_delete(O->second);
    }
	m_EditObjects.clear();
}
//----------------------------------------------------

void ELibrary::CleanLibrary()
{
	VERIFY(m_bReady);
    // remove all unused instance CEditableObject
    for(EditObjPairIt O = m_EditObjects.begin(); O!=m_EditObjects.end(); ){
    	if (0==O->second->m_RefCount){ 
        	EditObjPairIt D		= O; O++;
        	xr_delete			(D->second);
            m_EditObjects.erase	(D);
        }else					O++;
    }
}
//----------------------------------------------------
void ELibrary::ReloadObject(LPCSTR nm)
{
	VERIFY(m_bReady);
	R_ASSERT(nm&&nm[0]);
    string512 name; strcpy(name,nm); strlwr(name);
	EditObjPairIt it = m_EditObjects.find(name);
    if (it!=m_EditObjects.end())
    	it->second->Reload();
}
//---------------------------------------------------------------------------
void ELibrary::ReloadObjects(){
	VERIFY(m_bReady);
	EditObjPairIt O = m_EditObjects.begin();
	EditObjPairIt E = m_EditObjects.end();
    for(; O!=E; O++)
    	O->second->Reload();
}
//----------------------------------------------------

void ELibrary::OnDeviceCreate(){
	VERIFY(m_bReady);
	EditObjPairIt O = m_EditObjects.begin();
	EditObjPairIt E = m_EditObjects.end();
    for(; O!=E; O++)
    	O->second->OnDeviceCreate();
}
//---------------------------------------------------------------------------

void ELibrary::OnDeviceDestroy(){
	VERIFY(m_bReady);
	EditObjPairIt O = m_EditObjects.begin();
	EditObjPairIt E = m_EditObjects.end();
    for(; O!=E; O++)
    	O->second->OnDeviceDestroy();
}
//---------------------------------------------------------------------------

void ELibrary::EvictObjects()
{
	EditObjPairIt O = m_EditObjects.begin();
	EditObjPairIt E = m_EditObjects.end();
    for(; O!=E; O++)
    	O->second->EvictObject();
}
//----------------------------------------------------

CEditableObject* ELibrary::LoadEditObject(LPCSTR name)
{
	VERIFY(m_bReady);
    CEditableObject* m_EditObject = xr_new<CEditableObject>(name);
    string_path fn;
    FS.update_path(fn,_objects_,EFS.ChangeFileExt(name,".object").c_str());
    if (FS.exist(fn))
    {
        if (m_EditObject->Load(fn))	return m_EditObject;
    }else{
		ELog.Msg(mtError,"Can't find file '%s'",fn);
    }
    xr_delete(m_EditObject);
	return 0;
}
//---------------------------------------------------------------------------

CEditableObject* ELibrary::CreateEditObject(LPCSTR nm)
{
	VERIFY(m_bReady);
    R_ASSERT(nm&&nm[0]);
//.    UI->ProgressInfo		(nm);
    AnsiString name		= AnsiString(nm).LowerCase();
    // file exist - find in already loaded
    CEditableObject* m_EditObject = 0;
	EditObjPairIt it 	= m_EditObjects.find(name);
    if (it!=m_EditObjects.end())	m_EditObject = it->second;
    else if (0!=(m_EditObject=LoadEditObject(name.c_str())))
		m_EditObjects[name]	= m_EditObject;
    if (m_EditObject)	m_EditObject->m_RefCount++;
	return m_EditObject;
}
//---------------------------------------------------------------------------

void ELibrary::RemoveEditObject(CEditableObject*& object)
{
	if (object){
	    object->m_RefCount--;
    	R_ASSERT(object->m_RefCount>=0);
		if ((object->m_RefCount==0)&&EPrefs->object_flags.is(epoDiscardInstance))
			if (!object->IsModified()) UnloadEditObject(object->GetName());
        object=0;
	}
}
//---------------------------------------------------------------------------

void ELibrary::Save(FS_FileSet* modif_map)
{
	VERIFY(m_bReady);
	EditObjPairIt O = m_EditObjects.begin();
	EditObjPairIt E = m_EditObjects.end();
    if (modif_map)
    {
        for(; O!=E; O++)
        	if (modif_map->end()!=modif_map->find(FS_File(O->second->GetName())))
            {
                string_path 			nm;
                FS.update_path	(nm,_objects_,O->second->GetName());
                strcpy(nm, EFS.ChangeFileExt(nm,".object").c_str());

                if (!O->second->Save(nm))
                    Log			("!Can't save object:",nm);
            }
    }else
    {
        for(; O!=E; O++)
            if (O->second->IsModified())
            {
                string_path		nm;
                FS.update_path	(nm,_objects_,O->second->GetName());
                strcpy			(nm, EFS.ChangeFileExt(nm,".object").c_str());

                if (!O->second->Save(nm))
                    Log			("!Can't save object:",nm);
            }
    }
}
//---------------------------------------------------------------------------

int ELibrary::GetObjects(FS_FileSet& files)
{
    return FS.file_list(files,_objects_,FS_ListFiles|FS_ClampExt,"*.object");
}
//---------------------------------------------------------------------------

void ELibrary::RemoveObject(LPCSTR _fname, EItemType type, bool& res)   
{
	if (TYPE_FOLDER==type){
    	FS.dir_delete			(_objects_,_fname,FALSE);
        res 					= true;
		return;
    }else if (TYPE_OBJECT==type){
        string_path fname,src_name;
        strcpy(fname,EFS.ChangeFileExt(_fname,".object").c_str());
        FS.update_path			(src_name,_objects_,fname);
        if (FS.exist(src_name))
        {
            xr_string thm_name	= EFS.ChangeFileExt(fname,".thm");
            // source
            FS.file_delete		(src_name);
            // thumbnail
            FS.file_delete		(_objects_,thm_name.c_str());

	        UnloadEditObject	(_fname);
            
            res = true;
            return;
        }
    }else THROW;
    res = false;
}
//---------------------------------------------------------------------------

void ELibrary::RenameObject(LPCSTR nm0, LPCSTR nm1, EItemType type)
{
	if (TYPE_FOLDER==type){
    	FS.dir_delete			(_objects_,nm0,FALSE);
    }else if (TYPE_OBJECT==type){
        string_path fn0,fn1,temp;
        // rename base file
        FS.update_path(fn0,_objects_,nm0);
        strcat(fn0,".object");
        FS.update_path(fn1,_objects_,nm1);
        strcat(fn1,".object");
        FS.file_rename(fn0,fn1,false);

        // rename thm
        FS.update_path(fn0,_objects_,nm0);
        strcat(fn0,".thm");
        FS.update_path(fn1,_objects_,nm1);
        strcat(fn1,".thm");
        FS.file_rename(fn0,fn1,false);

        // rename in cache
        EditObjPairIt it 	= m_EditObjects.find(nm0);
	    if (it!=m_EditObjects.end()){
            m_EditObjects[nm1]	= it->second;
            m_EditObjects.erase	(it);
        }
	}
}
//---------------------------------------------------------------------------

void ELibrary::UnloadEditObject(LPCSTR full_name)
{
    EditObjPairIt it 	= m_EditObjects.find(full_name);
    if (it!=m_EditObjects.end()){
    	if (0!=it->second->m_RefCount){
        	ELog.DlgMsg(mtError,"Object '%s' still referenced.",it->first.c_str());
            THROW;
        }
    	m_EditObjects.erase(it);
    	xr_delete		(it->second);
    }
}
//---------------------------------------------------------------------------

