//---------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "SHCompilerTools.h"
#include "../xrEProps/folderlib.h"
#include "../xrEProps/ItemList.h"
//------------------------------------------------------------------------------

void CSHCompilerTools::FillItemList()
{
	// store folders
	RStrVec folders;
	Ext.m_Items->GetFolders(folders);
    // fill items
	ListItemsVec items;
    Shader_xrLCVec& lst = m_Library.Library();
    for (Shader_xrLCIt it=lst.begin(); it!=lst.end(); it++)
        LHelper().CreateItem(items,it->Name,0);
    // fill folders
    for (RStringVecIt s_it=folders.begin(); s_it!=folders.end(); s_it++)
        LHelper().CreateItem(items,**s_it,0);
    // assign items
	Ext.m_Items->AssignItems(items,false,true);
}

//------------------------------------------------------------------------------
void CSHCompilerTools::RealUpdateList()
{
	FillItemList			();
}
//------------------------------------------------------------------------------

void CSHCompilerTools::RealUpdateProperties()
{
	PropItemVec items;
	if (m_Shader){
		Shader_xrLC& L 		  	= *m_Shader;
		PHelper().CreateCName  	(items, "Name",					L.Name,  		sizeof(L.Name), m_CurrentItem);
        PHelper().CreateFloat 	(items, "Translucency",			&L.vert_translucency);
        PHelper().CreateFloat 	(items, "Ambient",				&L.vert_ambient);
        PHelper().CreateFloat 	(items, "LM density",			&L.lm_density,	0.01f,20.f);

        PHelper().CreateFlag32	(items, "Flags\\Collision",		&L.m_Flags,   	Shader_xrLC::flCollision);
        PHelper().CreateFlag32	(items, "Flags\\Rendering",		&L.m_Flags,   	Shader_xrLC::flRendering);
        PHelper().CreateFlag32	(items, "Flags\\OptimizeUV",	&L.m_Flags,   	Shader_xrLC::flOptimizeUV);
        PHelper().CreateFlag32	(items, "Flags\\Vertex light",	&L.m_Flags,   	Shader_xrLC::flLIGHT_Vertex);
        PHelper().CreateFlag32	(items, "Flags\\Cast shadow",	&L.m_Flags,   	Shader_xrLC::flLIGHT_CastShadow);
//.		PHelper().CreateFlag32	(items, "Flags\\Sharp",			&L.m_Flags,   	Shader_xrLC::flLIGHT_Sharp);
    }
    Ext.m_ItemProps->AssignItems		(items);
    Ext.m_ItemProps->SetModifiedEvent	(fastdelegate::bind<TOnModifiedEvent>(this,&CSHCompilerTools::Modified));
}
//---------------------------------------------------------------------------

 