#include "stdafx.h"
#include <lwrender.h>
#include <lwhost.h>
#include "../../../editors/ECore/Editor/editobject.h"
#include "../../../xrCore/FileSystem.h"
#include "../../../xrCore/FS.h"
#include "bone.h"
#include <lwdisplay.h>
#include <lwserver.h>

extern "C" LWItemInfo		*g_iteminfo;
extern "C" LWMessageFuncs	*g_msg;
extern "C" LWInterfaceInfo	*g_intinfo;
extern "C" HostDisplayInfo  *g_hdi;
extern "C" LWObjectInfo		*g_objinfo;

static BoneVec* m_LWBones=0;

static void AppendBone(LWItemID bone)
{
	m_LWBones->push_back(xr_new<CBone>());
	CBone* B = m_LWBones->back();
	B->SetName(g_iteminfo->name(bone));
	B->ParseBone(bone);
}

static void RecurseBone(LWItemID parent)
{
	LWItemID bone = g_iteminfo->firstChild(parent);
	while (bone!=LWITEM_NULL){
		if (g_iteminfo->type(bone)==LWI_BONE){
			AppendBone(bone);
			RecurseBone(bone);
		}
		bone = g_iteminfo->nextChild(parent,bone);
	}
}

static bool ParseObjectBones(LWItemID object, int& obj_cnt)
{
	LWItemID	bone, parent;
	bone		= g_iteminfo->first( LWI_BONE, object );

	if (!bone){
		g_msg->error("Can't find bone.",0);
		return false;
	}
	
	while (true){
		parent = g_iteminfo->parent(bone);
		if (!parent){
			g_msg->error("Can't find root bone.",0);
			return false;
		}
		if (g_iteminfo->type(parent)!=LWI_BONE) break;
		else									bone = parent;
	}

	if (bone){
		if (obj_cnt>0){
			g_msg->error("Can't support multiple objects.",0);
			return false;
		}
		AppendBone(bone);
		RecurseBone	(bone);

		obj_cnt++;
	}
	return true;
}

extern "C" {
	//-----------------------------------------------------------------------------------------
	void __cdecl SaveObject(GlobalFunc *global)
	{
		Core._initialize("XRayPlugin",ELogCallback,FALSE);
		FS._initialize	(CLocatorAPI::flScanAppRoot,NULL,"xray_path.ltx");

		// get bone ID
		LWItemID		object;
		bool bErr		= true;

		string_path buf	= "";
		object			= g_iteminfo->first( LWI_OBJECT, NULL );
		int obj_cnt		= 0;
		bool bObjSel	= false;

		while (object){
			if(g_intinfo->itemFlags(object)&LWITEMF_SELECTED){
				bObjSel = true;
				if (EFS.GetSaveName("$import$",buf,0,0)){
					bErr = false;

					char name[1024];
					_splitpath( buf, 0, 0, name, 0 );

					CEditableObject* obj = xr_new<CEditableObject>(name);
					obj->SetVersionToCurrent(TRUE,TRUE);

					// parse bone if exist
					bool bBoneExists=false;
					if (g_iteminfo->first( LWI_BONE, object )){
						m_LWBones = &obj->Bones();
						bBoneExists = true;
						if (!ParseObjectBones(object,obj_cnt)) bErr = true;
						if (bErr){
							// default bone part
							obj->BoneParts().push_back(SBonePart());
							SBonePart& BP = obj->BoneParts().back();
							BP.alias = "default";
							for (int b_i=0; b_i<(int)obj->Bones().size(); b_i++)
								BP.bones.push_back(obj->Bones()[b_i]->Name());
						}
					}
					if (!bErr){
						LPCSTR lwo_nm=g_objinfo->filename(object);
						{// append path
							string_path		path,dr,di;
							_splitpath		(lwo_nm,dr,di,0,0);
							strconcat		(sizeof(path),path,dr,di);                                       
							if (!FS.path_exist(path)) FS.append_path(path,path,0,FALSE);
						}
						if (FS.exist(lwo_nm)){
							if (!obj->Import_LWO(lwo_nm,false)) bErr = true;
							else{ 
								obj->m_Flags.set(CEditableObject::eoDynamic,TRUE);
								obj->Optimize	();
								obj->SaveObject	(buf);
							}
						}else
							bErr = true;
					}
					// перенести выше или проверить не перетерает ли инфу о костях

					if (bErr)	g_msg->error("Export failed.",0);
					else		g_msg->info	("Export successful.",buf);
					bErr = false;

					xr_delete(obj);
					m_LWBones = 0;
					//				freeObjectDB(odb);
					break;
				}
			}
			object = g_iteminfo->next( object );
		}
		if (!bObjSel)	g_msg->error("Select object at first.",0);
		else if (bErr)	g_msg->error("Export failed.",0);
	}
//-----------------------------------------------------------------------------------------
};