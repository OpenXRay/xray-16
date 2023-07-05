//----------------------------------------------------
// file: CEditableObjectImport.cpp
//----------------------------------------------------

#include "stdafx.h"
#pragma hdrstop

//#include "lwo2.h"
#include "EditMesh.h"

extern "C"
{
#include "lwo2.h"
};

#ifdef _EDITOR

extern "C" __declspec(dllimport) lwObject *LWO_ImportObject(char *filename, lwObject *new_obj);
extern "C" __declspec(dllimport) void LWO_CloseFile(lwObject *new_obj);
#endif

DEFINE_MAP(void *, int, VMIndexLink, VMIndexLinkIt);

bool CompareFunc(const st_VMapPt &vm0, const st_VMapPt &vm1)
{
	return vm0.vmap_index < vm1.vmap_index;
};
