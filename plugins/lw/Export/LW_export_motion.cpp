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

static COMotion* m_Motion;

void SelectedCount(LWItemType type, int& sel_obj_count, LWItemID& last_sel_obj)
{
	LWItemID	object;
	object		= g_iteminfo->first( type, NULL );
	while ( object ) {
		if(g_intinfo->itemFlags(object)&LWITEMF_SELECTED){
			last_sel_obj	= object;
			sel_obj_count++;
		}
		object	= g_iteminfo->next( object );
	}
}

extern "C" {
//-----------------------------------------------------------------------------------------
void __cdecl SaveObjectMotion(GlobalFunc *global)
{
	Core._initialize("XRayPlugin",ELogCallback,FALSE);
	FS._initialize	(CLocatorAPI::flScanAppRoot,NULL,"xray_path.ltx");
	// get bone ID
	bool bErr		= false;

	string_path buf	= "";

	EFS.GetSaveName	("$omotion$",buf);
	if (buf[0]){
		int sel_obj_cnt	= 0;
		LWItemID sel_object;

		SelectedCount	(LWI_OBJECT,sel_obj_cnt,sel_object);
		SelectedCount	(LWI_CAMERA,sel_obj_cnt,sel_object);
		SelectedCount	(LWI_BONE,	sel_obj_cnt,sel_object);
		SelectedCount	(LWI_LIGHT,	sel_obj_cnt,sel_object);

		if (sel_obj_cnt==1){
			string_path					name;
			_splitpath					(buf, 0, 0, name, 0);
			m_Motion					= xr_new<COMotion>();
			m_Motion->SetName			(name);
			m_Motion->ParseObjectMotion	(sel_object);
			m_Motion->SetParam			(g_intinfo->previewStart, g_intinfo->previewEnd, (float)g_lwsi->framesPerSecond);
			m_Motion->SaveMotion		(buf);
			g_msg->info					("Export motion successful.",g_iteminfo->name(sel_object));
			xr_delete					(m_Motion);
		}else{
			g_msg->error				("Select one object and try again.",NULL);
		}
	}else{
		g_msg->error("Export failed. Empty name.",NULL);
	}
}
};