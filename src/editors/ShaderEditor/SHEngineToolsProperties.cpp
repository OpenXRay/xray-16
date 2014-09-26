//---------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "SHEngineTools.h"
#include "../../Layers/xrRender/blenders/Blender.h"
#include "../xrEProps/PropertiesList.h"
#include "../xrEProps/folderlib.h"
#include "../ECore/Editor/ui_main.h"

//---------------------------------------------------------------------------
xr_token							mode_token					[ ]={
	{ "Programmable",			  	CMatrix::modeProgrammable  	},
	{ "TCM",					  	CMatrix::modeTCM		  	},
	{ "Spherical Reflection",	  	CMatrix::modeS_refl	   		},
	{ "Cube Reflection",			CMatrix::modeC_refl	  		},
	{ 0,							0							}           
};
//---------------------------------------------------------------------------
void __fastcall CSHEngineTools::FillMatrixProps(PropItemVec& items, LPCSTR pref, LPSTR name)
{
    CMatrix* M 						= AppendMatrix(name);
    R_ASSERT(M);

    PHelper().CreateToken32			(items,PrepareKey(pref,"Mode"),&M->dwMode,mode_token);

    if (M->dwMode==CMatrix::modeTCM){
	    PHelper().CreateFlag32		(items,	PrepareKey(pref,"Scale enabled"),	&M->tcm_flags,CMatrix::tcmScale);
		PHelper().CreateWave		(items,	PrepareKey(pref,"Scale U"),			&M->scaleU);
		PHelper().CreateWave		(items,	PrepareKey(pref,"Scale V"),			&M->scaleV);
	    PHelper().CreateFlag32		(items,	PrepareKey(pref,"Rotate enabled"),	&M->tcm_flags,CMatrix::tcmRotate);
		PHelper().CreateWave		(items,	PrepareKey(pref,"Rotate"),			&M->rotate);
	    PHelper().CreateFlag32		(items,	PrepareKey(pref,"Scroll enabled"),	&M->tcm_flags,CMatrix::tcmScroll);
		PHelper().CreateWave		(items,	PrepareKey(pref,"Scroll U"),		&M->scrollU);
		PHelper().CreateWave		(items,	PrepareKey(pref,"Scroll V"),		&M->scrollV);
    }
}
//---------------------------------------------------------------------------

void __fastcall CSHEngineTools::MCOnDraw(PropValue* sender, xr_string& draw_val)
{
	if (draw_val[0]!='$') draw_val="Custom";
}
//---------------------------------------------------------------------------

bool CSHEngineTools::MatrixOnAfterEdit(PropValue* sender, xr_string& nm)
{
	CListValue* V 	= dynamic_cast<CListValue*>(sender);  R_ASSERT(V);
	VERIFY			(nm.size());
    LPCSTR src_val 	= V->GetValue();

	if (nm[0]!='$'){
        if (src_val[0]=='$'){
            nm	= AppendMatrix();
        }else{
            nm	= src_val;
        }
    }else{                       
        if (src_val[0]!='$'){
            RemoveMatrix	(src_val);
            V->ApplyValue	(src_val);
        }
    }
    return true;
}
//------------------------------------------------------------------------------

void __fastcall CSHEngineTools::FillConstProps(PropItemVec& items, LPCSTR pref, LPSTR name)
{
	CConstant* C = AppendConstant(name);
    R_ASSERT(C);
    PHelper().CreateWave(items,PrepareKey(pref,"R"),&C->_R);
    PHelper().CreateWave(items,PrepareKey(pref,"G"),&C->_G);
    PHelper().CreateWave(items,PrepareKey(pref,"B"),&C->_B);
    PHelper().CreateWave(items,PrepareKey(pref,"A"),&C->_A);
}
//---------------------------------------------------------------------------

bool CSHEngineTools::ConstOnAfterEdit(PropValue* sender, xr_string& nm)
{
	CListValue* V 	= dynamic_cast<CListValue*>(sender);  R_ASSERT(V);
    VERIFY			(nm.size());
    LPCSTR src_val 	= V->GetValue();

	if (nm[0]!='$'){
        if (src_val[0]=='$'){
            nm = AppendConstant();
        }else{
            nm = src_val;
        }
    }else{
        if (src_val[0]!='$'){
            RemoveConstant(src_val);
            V->ApplyValue(src_val);
        }
    }
    return true;
}
//------------------------------------------------------------------------------
bool CSHEngineTools::NameOnAfterEdit(PropValue* sender, xr_string& new_name)
{
	CTextValue* V 			= dynamic_cast<CTextValue*>(sender); R_ASSERT(V);
    AnsiString nn			= new_name.c_str();
	if (FHelper.NameAfterEdit((TElTreeItem*)m_CurrentItem->Item(),V->GetValue(),nn)){
    	new_name			= nn.c_str();
    	RemoteRenameBlender(V->GetValue(),new_name.c_str());
    }
    return true;
}
//------------------------------------------------------------------------------

void CSHEngineTools::RealUpdateList()
{
	if (m_bFreezeUpdate) 	return;
	FillItemList			();
}
//------------------------------------------------------------------------------

void CSHEngineTools::RealUpdateProperties()
{
	if (m_bFreezeUpdate) return;

	PropItemVec 	items;
	if (m_CurrentBlender){ // fill Tree
    	AnsiString marker_text="";
    
    	IReader data(m_BlenderStream.pointer(), m_BlenderStream.size());
        CBlender_DESC* desc=(CBlender_DESC*)data.pointer();
        data.advance(sizeof(CBlender_DESC));
        DWORD type;
        string256 key;

        PHelper().CreateCaption(items,"Type",m_CurrentBlender->getComment());
        PHelper().CreateCaption(items,"Owner",desc->cComputer);
		CTextValue* V = PHelper().CreateCText(items,"Name",desc->cName,sizeof(desc->cName));
		V->OnAfterEditEvent.bind		(this,&CSHEngineTools::NameOnAfterEdit);
		V->OnBeforeEditEvent.bind		(&PHelper(),&IPropHelper::CNameBeforeEdit);
		V->Owner()->OnDrawTextEvent.bind(&PHelper(),&IPropHelper::CNameDraw);

        while (!data.eof()){
            int sz=0;
            type = data.r_u32();     
            data.r_stringZ(key,sizeof(key));
            switch(type){
            case xrPID_MARKER:
	            marker_text = key;
            break;
            case xrPID_TOKEN:{
            	xrP_TOKEN* V	= (xrP_TOKEN*)data.pointer();
            	sz				= sizeof(xrP_TOKEN)+sizeof(xrP_TOKEN::Item)*V->Count;
                PHelper().CreateTokenSH(items,PrepareKey(marker_text.c_str(),key),&V->IDselected,(TokenValueSH::Item*)(LPBYTE(data.pointer()) + sizeof(xrP_TOKEN)),V->Count);
            }break;
            case xrPID_MATRIX:{
            	sz				= sizeof(string64);
                LPSTR V			= (LPSTR)data.pointer();
                CListValue* P	= PHelper().CreateCList(items,PrepareKey(marker_text.c_str(),key),V,sz,&*MCString.begin(),MCString.size());
                AnsiString pref = AnsiString(PrepareKey(marker_text.c_str(),"Custom ").c_str())+key;
				if (V&&V[0]&&(*V!='$')) FillMatrixProps(items,pref.c_str(),V);
				P->OnAfterEditEvent.bind(this,&CSHEngineTools::MatrixOnAfterEdit);
				P->Owner()->OnDrawTextEvent.bind(this,&CSHEngineTools::MCOnDraw);
            }break;
            case xrPID_CONSTANT:{
            	sz=sizeof(string64);
            	sz				= sizeof(string64);
                LPSTR V			= (LPSTR)data.pointer();
                CListValue* P	= PHelper().CreateCList(items,PrepareKey(marker_text.c_str(),key),V,sz,&*MCString.begin(),MCString.size());
                AnsiString pref = AnsiString(PrepareKey(marker_text.c_str(),"Custom ").c_str())+key;
				if (V&&V[0]&&(*V!='$')) FillConstProps(items,pref.c_str(),V);
				P->OnAfterEditEvent.bind(this,&CSHEngineTools::ConstOnAfterEdit);
				P->Owner()->OnDrawTextEvent.bind(this,&CSHEngineTools::MCOnDraw);
            }break;
            case xrPID_TEXTURE:
            	sz=sizeof(string64);
				PHelper().CreateTexture(items,PrepareKey(marker_text.c_str(),key),(LPSTR)data.pointer(),sz);
            break;
            case xrPID_INTEGER:{
            	sz=sizeof(xrP_Integer);
                xrP_Integer* V=(xrP_Integer*)data.pointer();
                PHelper().CreateS32(items,PrepareKey(marker_text.c_str(),key),&V->value,V->min,V->max,1);
            }break;
            case xrPID_FLOAT:{
            	sz=sizeof(xrP_Float);
                xrP_Float* V=(xrP_Float*)data.pointer();
                PHelper().CreateFloat(items,PrepareKey(marker_text.c_str(),key),&V->value,V->min,V->max,0.01f,2);
            }break;
            case xrPID_BOOL:{
            	sz=sizeof(xrP_BOOL);
                xrP_BOOL* V=(xrP_BOOL*)data.pointer();
                PHelper().CreateBOOL(items,PrepareKey(marker_text.c_str(),key),&V->value);
            }break;
            default: THROW2("UNKNOWN xrPID_????");
            }
            data.advance(sz);
        }
// גלוסעמ ApplyChanges(true);
        UpdateObjectFromStream();
        Ext.m_ItemProps->ResetModified();
//---------------------------
    }
    Ext.m_ItemProps->AssignItems(items);
    Ext.m_ItemProps->SetModifiedEvent(fastdelegate::bind<TOnModifiedEvent>(this,&CSHEngineTools::Modified));
}
//------------------------------------------------------------------------------

