#include "stdafx.h"
#pragma hdrstop

#include "PropertiesEObject.h"
#include "SceneObject.h"
#include "../ECore/Editor/EditObject.h"
#include "../ECore/Editor/EditMesh.h"
#include "../ECore/Editor/EThumbnail.h"
#include "../ECore/Editor/ui_main.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "ElXPThemedControl"
#pragma link "ExtBtn"
#pragma link "mxPlacemnt"
#pragma link "ExtBtn"
#pragma link "ElPgCtl"
#pragma link "ElXPThemedControl"
#pragma link "MXCtrls"
#pragma resource "*.dfm"
//---------------------------------------------------------------------------
__fastcall TfrmPropertiesEObject::TfrmPropertiesEObject(TComponent* Owner)
    : TForm(Owner)
{
	m_Thumbnail	= 0;
//    m_pEditObject=0;
}
//---------------------------------------------------------------------------


TfrmPropertiesEObject* TfrmPropertiesEObject::CreateProperties(TWinControl* parent, TAlign align, TOnModifiedEvent modif)
{
	TfrmPropertiesEObject* props = xr_new<TfrmPropertiesEObject>(parent);
    props->OnModifiedEvent = modif;
    if (parent){
		props->Parent 	= parent;
    	props->Align 	= align;
	    props->BorderStyle = bsNone;
        props->ShowProperties();
    }
    props->m_BasicProp	= TProperties::CreateForm("",props->paBasic,alClient,props->OnModifiedEvent);
    props->m_SurfProp 	= TProperties::CreateForm("",props->paSurfaces,alClient,props->OnModifiedEvent,TOnILItemFocused(props,&TfrmPropertiesEObject::OnSurfaceFocused));
	return props;
}
//---------------------------------------------------------------------------

void TfrmPropertiesEObject::DestroyProperties(TfrmPropertiesEObject*& props)
{
	VERIFY(props);
	props->Close();
    xr_delete(props);
}
//---------------------------------------------------------------------------

void TfrmPropertiesEObject::FillBasicProps()
{
	xr_vector<CSceneObject*>::iterator it = m_pEditObjects.begin();
	xr_vector<CSceneObject*>::iterator it_e = m_pEditObjects.end();
	PropItemVec items;
    for(;it!=it_e;++it)
    {
	// basic
	CSceneObject* 		S = *it;
    if (S->GetReference())
    {
    	CEditableObject* 	O = S->GetReference();
        O->FillBasicProps	(0,items);
    }
    }
	m_BasicProp->AssignItems(items);
}
//---------------------------------------------------------------------------

void TfrmPropertiesEObject::FillSurfProps()
{
	xr_vector<CSceneObject*>::iterator it = m_pEditObjects.begin();
	xr_vector<CSceneObject*>::iterator it_e = m_pEditObjects.end();
    PropItemVec values;
    for(;it!=it_e;++it)
    {
        // surfaces
        CSceneObject* 		S = *it;
        if (S->GetReference())
        {
            CEditableObject* 	O = S->GetReference();
            for (SurfaceIt it=O->FirstSurface(); it!=O->LastSurface(); it++)
            {
                AnsiString	pref	= AnsiString("Surfaces\\")+(*it)->_Name();
                PropValue* V		= PHelper().CreateCaption(values,pref.c_str(),"");
                V->tag				= (int)*it;
                O->FillSurfaceProps	(*it,pref.c_str(),values);
            }
        }
    }
    m_SurfProp->AssignItems(values);
}
//---------------------------------------------------------------------------

void TfrmPropertiesEObject::UpdateProperties(xr_vector<CSceneObject*> Ss, bool bReadOnly)
{
	m_BasicProp->SetReadOnly(bReadOnly);
	m_SurfProp->SetReadOnly	(bReadOnly);
//.	m_pEditObject 			= S;
	m_pEditObjects 			= Ss;
	FillBasicProps			();
    FillSurfProps			();
}

void __fastcall TfrmPropertiesEObject::FormDestroy(TObject *Sender)
{
	xr_delete(m_Thumbnail);
    TProperties::DestroyForm(m_BasicProp);
    TProperties::DestroyForm(m_SurfProp);
}
//---------------------------------------------------------------------------

void __fastcall TfrmPropertiesEObject::fsStorageRestorePlacement(
      TObject *Sender)
{
	m_BasicProp->RestoreParams(fsStorage);
	m_SurfProp->RestoreParams(fsStorage);
}
//---------------------------------------------------------------------------

void __fastcall TfrmPropertiesEObject::fsStorageSavePlacement(
      TObject *Sender)
{
	m_BasicProp->SaveParams(fsStorage);
	m_SurfProp->SaveParams(fsStorage);
}
//---------------------------------------------------------------------------

void TfrmPropertiesEObject::OnSurfaceFocused(TElTreeItem* item)
{
	xr_delete(m_Thumbnail);

	if (item&&item->Tag)
    {
        PropItem* prop		= (PropItem*)item->Tag;
    	EPropType type		= TProperties::GetItemType(item);
    	switch (type)
        {
        	case PROP_CAPTION:
            {
    			if(m_pEditObjects.size()==1)
                {
                    PropValue* V		= prop->GetFrontValue(); VERIFY(V);
                    CSceneObject* O 	= m_pEditObjects[0];
                    CSurface* S			= (CSurface*)V->tag;
                    O->Blink			(S);
                }
            }break;
        	case PROP_CHOOSE:
            {
		    	ChooseValue* V		= dynamic_cast<ChooseValue*>(prop->GetFrontValue()); VERIFY(V);
                if (smTexture==V->m_ChooseID)
                {
                    LPCSTR nm 				= TProperties::GetItemColumn(item,0);
                    if (nm&&nm[0])
                    {
                        m_Thumbnail = xr_new<ETextureThumbnail>(nm);
                        lbWidth->Caption 	= m_Thumbnail->_Width();
                        lbHeight->Caption 	= m_Thumbnail->_Height();
                        lbAlpha->Caption 	= (m_Thumbnail->_Alpha())?"present":"absent";
                        if (m_Thumbnail->_Width()!=m_Thumbnail->_Height()) paImage->Repaint();
                        paImage->Repaint	();
                    }
                }
            }break;
        }
    }
    if (!m_Thumbnail)
    {
        lbWidth->Caption 		= "...";
        lbHeight->Caption 		= "...";
        lbAlpha->Caption 		= "...";
        paImage->Repaint();
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmPropertiesEObject::paImagePaint(TObject *Sender)
{
	if (m_Thumbnail) m_Thumbnail->Draw(paImage);
}
//---------------------------------------------------------------------------

void __fastcall TfrmPropertiesEObject::OnPick(const SRayPickInfo& pinf)
{
	R_ASSERT(pinf.e_mesh);
	if (ebDropper->Down && /*m_pEditObject*/ m_pEditObjects.size())
    {
        CSurface* surf=pinf.e_mesh->GetSurfaceByFaceID(pinf.inf.id);
        AnsiString s_name = AnsiString("Surfaces\\")+AnsiString(surf->_Name());
        FHelper.RestoreSelection(m_SurfProp->tvProperties,s_name,false);
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmPropertiesEObject::FormShow(TObject *Sender)
{
	// check window position
    UI->CheckWindowPos(this);
}
//---------------------------------------------------------------------------

                           
