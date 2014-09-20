#include "stdafx.h"
#include "../../../xrCore/FileSystem.h"
#include "../../../xrCore/FS.h"
#include <lwrender.h>
#include <lwhost.h>
#include "Envelope.h"
#include "Bone.h"
#include "Motion.h"
#include "scenscan\objectdb.h"
#include <lwdisplay.h>

extern "C"	LWItemInfo		*g_iteminfo;
extern "C"	LWChannelInfo	*g_chinfo;
extern "C"	LWEnvelopeFuncs	*g_envf;
extern "C"	LWSceneInfo		*g_lwsi;
extern "C"	LWInterfaceInfo	*g_intinfo;
extern "C"	LWMessageFuncs	*g_msg;
extern "C"	LWBoneInfo		*g_boneinfo;
extern "C"	LWObjectFuncs	*g_objfunc;
extern "C"	LWObjectInfo	*g_objinfo;
extern "C"	HostDisplayInfo *g_hdi;

static CSMotion* m_Motion;

static void RecurseBone(LWItemID parent){
	LWItemID bone = g_iteminfo->firstChild(parent);
	while (bone!=LWITEM_NULL){
		if (g_iteminfo->type(bone)==LWI_BONE){
			m_Motion->ParseBoneMotion(bone);
			RecurseBone(bone);
		}
		bone = g_iteminfo->nextChild(parent,bone);
	}
}

static bool ParseObjectMotion(LWItemID object, int& obj_cnt){
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
		m_Motion->ParseBoneMotion(bone);
		RecurseBone	(bone);

		obj_cnt++;
	}
	return true;
}

void ReplaceSpaceAndLowerCase(shared_str& s)
{
	if (*s){
		char* _s	= xr_strdup(*s);
		char* lp	= _s;
		while(lp[0]){if (lp[0]==' ') lp[0]='_'; lp++;}
		xr_strlwr	(_s);
		s			= _s;
		xr_free		(_s);
	}
}

extern "C" {
//-----------------------------------------------------------------------------------------
void __cdecl SaveSkeletonMotion(GlobalFunc *global)
{
	Core._initialize("XRayPlugin",ELogCallback,FALSE);
	FS._initialize	(CLocatorAPI::flScanAppRoot,NULL,"xray_path.ltx");
	// get bone ID
	LWItemID		object;
	bool bErr		= true;
	
	string_path		buf="";
	string64		name;

	EFS.GetSaveName	("$smotion$",buf);

	if (buf[0]){
		object		= g_iteminfo->first( LWI_OBJECT, NULL );
		int obj_cnt = 0;
		_splitpath( buf, 0, 0, name, 0 );
		m_Motion	= xr_new<CSMotion>();
		m_Motion->SetName(name);
		while ( object ) {
			if(g_intinfo->itemFlags(object)&LWITEMF_SELECTED){
				bErr = !ParseObjectMotion(object,obj_cnt); 
				break;
			}
			object = g_iteminfo->next( object );
		}
		
		if (!bErr){	
			m_Motion->SetParam(g_intinfo->previewStart, g_intinfo->previewEnd, (float)g_lwsi->framesPerSecond);
			m_Motion->SaveMotion(buf);
			g_msg->info	("Export successful.",buf);
		}else g_msg->error("Export failed.",0);
		
		xr_delete(m_Motion);
	}
}
};