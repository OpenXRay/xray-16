#include "stdafx.h"
#pragma hdrstop

#include "SceneSummaryInfo.h"
#include "../ECore/Editor/ImageManager.h"
#include "../ECore/Editor/EThumbnail.h"
#include "SceneSummaryInfo.h"
#include "Scene.h"
#include "SceneObject.h"
#include "../ECore/Editor/ui_main.h"
#include "ui_levelmain.h"
#include "../ECore/Editor/Library.h"

static SSceneSummary s_summary;

xr_token summary_texture_type_tokens[]={
	{ "Base",			SSceneSummary::sttBase		},           
	{ "Implicit",		SSceneSummary::sttImplicit	},
	{ "DO",				SSceneSummary::sttDO		},
	{ "Glow",			SSceneSummary::sttGlow		},
	{ "LOD",			SSceneSummary::sttLOD		},
	{ 0,				0							}
};

void SSceneSummary::Prepare()
{
	for (TISetIt t_it=textures.begin(); t_it!=textures.end(); t_it++){
        STextureInfo* info	= (STextureInfo*)(&(*t_it));
    	if (!info->bReady) 	info->Prepare();
    }
	for (OISetIt o_it=objects.begin(); o_it!=objects.end(); o_it++){
        SObjectInfo* info	= (SObjectInfo*)(&(*o_it));
    	if (!info->bReady) 	info->Prepare();
    }
}

xr_string _itoa(int val)
{
	string64 	tmp; 
    return 		itoa(val,tmp,10);
}
void SSceneSummary::SObjectInfo::Prepare()
{
	bReady					= true;
	if (object_name.size()){
	    CEditableObject* O	= Lib.CreateEditObject(object_name.c_str());
        xr_string pref		= object_name.c_str();
        if (O){
        	SPairInfo 		tmp;
            tmp.first		= pref+"\\References"; 			
            tmp.second 		= _itoa(ref_count);					
            info.push_back	(tmp);
            tmp.first		= pref+"\\Geometry\\Faces"; 	tmp.second = _itoa(O->GetFaceCount());			info.push_back	(tmp);
            tmp.first		= pref+"\\Geometry\\Vertices"; 	tmp.second = _itoa(O->GetVertexCount());		info.push_back	(tmp);
            SurfaceVec& surfaces = O->Surfaces();
            for (SurfaceIt it=surfaces.begin(); it!=surfaces.end(); it++){
            	xr_string pr= pref+xr_string("\\Materials\\")+(*it)->_Name();
	            tmp.first	= pr+"\\Texture"; 		tmp.second = (*it)->_Texture();							info.push_back	(tmp);
	            tmp.first	= pr+"\\Faces"; 		tmp.second = _itoa(O->GetSurfFaceCount((*it)->_Name()));info.push_back	(tmp);
            }
	    	Lib.RemoveEditObject(O);
        }
    }else{
        Msg("!Empty object name found.");
    }
}
void SSceneSummary::SObjectInfo::FillProp(PropItemVec& items, LPCSTR main_pref)
{
	if (object_name.size()){
        for (PIVecIt it=info.begin(); it!=info.end(); it++)
	        PHelper().CreateCaption(items,PrepareKey(main_pref,it->first.c_str()),	it->second.c_str());
    }
}
void SSceneSummary::SObjectInfo::Export	(IWriter* F)
{
	string1024		tmp;
    for (PIVecIt it=info.begin(); it!=info.end(); it++){
        sprintf		(tmp,"%s=%s",it->first.c_str(),it->second.c_str());
		F->w_string	(tmp);
    }
}

void SSceneSummary::STextureInfo::Prepare	()
{
	bReady					= true;
	if (file_name.size()){
        ETextureThumbnail* T= (ETextureThumbnail*)ImageLib.CreateThumbnail(file_name.c_str(),ECustomThumbnail::ETTexture,true);
        if (!T->Valid()){
            Msg("!Can't get info from texture: '%s'",file_name.c_str());
        }else{
            info			= T->_Format();
            if (info.flags.is(STextureParams::flImplicitLighted))
	            type 		= sttImplicit;
        }
        xr_delete			(T);
    }else{
        Msg("!Empty texture name found.");
    }
}

void SSceneSummary::STextureInfo::OnHighlightClick(ButtonValue* sender, bool& bDataModified, bool& bSafe)
{
	ButtonValue* V = dynamic_cast<ButtonValue*>(sender); R_ASSERT(V);
    AnsiString item_name = sender->Owner()->Item()->Text;
    switch (V->btn_num){
    case 0: Scene->HighlightTexture	((LPCSTR)sender->tag,false,info.width,info.height,false); break;
    case 1: Scene->HighlightTexture	((LPCSTR)sender->tag,true,info.width,info.height,false); break;
    case 2: Scene->HighlightTexture	((LPCSTR)sender->tag,true,info.width,info.height,true); break;
    case 3: ExecCommand(COMMAND_CLEAR_DEBUG_DRAW); break;
	}
    bDataModified 	= false;
    bSafe 			= false;
}

void SSceneSummary::STextureInfo::FillProp	(PropItemVec& items, LPCSTR main_pref, u32& mem_use)
{
	if (file_name.size()){
        int tex_mem			= info.MemoryUsage(*file_name);
        mem_use				+= tex_mem;
        AnsiString pref		= PrepareKey(AnsiString(main_pref).c_str(),*file_name).c_str();
        PropValue* V=0;
        V=PHelper().CreateChoose(items,PrepareKey(pref.c_str(),"Texture"), 		&file_name, smTexture); V->Owner()->Enable(FALSE);
        PHelper().CreateCaption(items,PrepareKey(pref.c_str(),"Format"),		info.FormatString());
        PHelper().CreateCaption(items,PrepareKey(pref.c_str(),"Size"), 			shared_str().printf("%d x %d x %s",info.width,info.height,info.HasAlpha()?"32b":"24b"));
        PHelper().CreateCaption(items,PrepareKey(pref.c_str(),"Memory Usage"),	shared_str().printf("%d Kb",iFloor(tex_mem/1024)));
        PHelper().CreateCaption(items,PrepareKey(pref.c_str(),"Effective Area"),shared_str().printf("%3.2f m^2",effective_area));
        PHelper().CreateCaption(items,PrepareKey(pref.c_str(),"Pixel Density"),	shared_str().printf("%3.2f p/m",_sqrt((pixel_area*info.width*info.height)/effective_area)));
/*
//. убрал из-за кол-ва > 4096 
        AnsiString tmp 		= "on demand";
        for (objinf_map_it o_it=objects.begin(); o_it!=objects.end(); o_it++){
        	tmp += AnsiString().sprintf("%s%s[%d*%3.2f]",tmp.Length()?"; ":"",o_it->first.c_str(),o_it->second.ref_count,o_it->second.area);
        }
        PHelper().CreateCaption(items,PrepareKey(pref.c_str(),"Objects"), tmp.c_str());
*/
        if (info.flags.is_any(STextureParams::flDiffuseDetail|STextureParams::flBumpDetail)){
            if (0!=info.detail_name.size()){
                V=PHelper().CreateChoose(items,PrepareKey(pref.c_str(),"Detail Texture"),	&info.detail_name,smTexture); 	V->Owner()->Enable(FALSE);
                PHelper().CreateCaption(items,PrepareKey(pref.c_str(), "Detail Scale"),		shared_str().printf("%3.2f",info.detail_scale));
            }else{
                PHelper().CreateCaption(items,PrepareKey(pref.c_str(), "Detail Texture"),	"INVALID");
                ELog.Msg(mtError,"Empty details on texture: '%s'",*file_name);
            }
        }
        if (info.bump_mode==STextureParams::tbmUse){
            if (0!=info.bump_name.size()){
                V=PHelper().CreateChoose(items,PrepareKey(pref.c_str(),"Bump Texture"),		&info.bump_name,smTexture); 	V->Owner()->Enable(FALSE);
            }else{
                PHelper().CreateCaption(items,PrepareKey(pref.c_str(), "Bump Texture"),		"INVALID");    
                ELog.Msg(mtError,"Empty bump on texture: '%s'",*file_name);
            }
        }
        ButtonValue* B 		= PHelper().CreateButton(items,PrepareKey(pref.c_str(),"Highlight Texture"), "Select,Density =,Density +,Clear", 0);
		B->OnBtnClickEvent.bind(this,&SSceneSummary::STextureInfo::OnHighlightClick);
        B->tag 				= (int)(*file_name);
    }
}
void SSceneSummary::STextureInfo::Export	(IWriter* F, u32& mem_use)
{
	string128		mask;
	AnsiString		tmp;
    strcpy			(mask,"%s=%s,%d,%d,%s,%d,%3.2f,%3.2f,%s");
    if (info.flags.is_any(STextureParams::flDiffuseDetail|STextureParams::flBumpDetail)){
        if (0!=info.detail_name.size()){
        	strcat	(mask,",%s,%3.2f");
        }
    }
    if (info.bump_mode==STextureParams::tbmUse){
        if (0!=info.bump_name.size()){
        	strcat	(mask,",%s");
        }
    }
    AnsiString 		tmp2;
    for (objinf_map_it o_it=objects.begin(); o_it!=objects.end(); o_it++){
        tmp2 		+= AnsiString().sprintf("%s%s[%d*%3.2f]",tmp2.Length()?"; ":"",o_it->first.c_str(),o_it->second.ref_count,o_it->second.area);
    }
    int tex_mem		= info.MemoryUsage(*file_name);
    mem_use			+=tex_mem;
    tmp.sprintf		(mask,*file_name,info.FormatString(),
    				info.width,info.height,info.HasAlpha()?"present":"absent",
                    iFloor(tex_mem/1024),
                    effective_area, _sqrt((pixel_area*info.width*info.height)/effective_area), tmp2.c_str(), 
                    *info.detail_name, info.detail_scale, *info.bump_name);
	F->w_string		(tmp.c_str());
}

void SSceneSummary::OnFileClick(ButtonValue* sender, bool& bModif, bool& bSafe)
{
	ButtonValue* V = dynamic_cast<ButtonValue*>(sender); R_ASSERT(V);
    switch (V->btn_num){
    case 0:{
    	xr_string fn = Scene->m_LevelOp.m_FNLevelPath.c_str();
    	if (EFS.GetSaveName(_import_,fn,0,2))
	    	if (ExportSummaryInfo(fn.c_str())) 	ELog.DlgMsg(mtInformation,"Export completed.");
            else								ELog.DlgMsg(mtInformation,"Export failed.");
    }break;
	}
    bModif = false;
}
void SSceneSummary::OnHighlightClick(ButtonValue* V, bool& bDataModified, bool& bSafe)
{
    AnsiString item_name = V->Owner()->Item()->Text;
    switch (V->btn_num){
    case 0:{ 
    	ExecCommand				(COMMAND_CLEAR_DEBUG_DRAW);
        for (TISetIt it=textures.begin(); it!=textures.end(); it++){
            STextureInfo* info	= (STextureInfo*)(&(*it));
            if (info->type==sttBase)
		    	Scene->HighlightTexture	(info->file_name.c_str(),true,info->info.width,info->info.height,true); 
        }
    }break;
    case 1: ExecCommand(COMMAND_CLEAR_DEBUG_DRAW); break;
	}
    bDataModified 	= false;
    bSafe 			= false;
    
    
}

bool SSceneSummary::ExportSummaryInfo(LPCSTR fn)
{
	IWriter* F 				= FS.w_open(fn); 
    if (F){
        string256				tmp;
        // textures
        u32 total_mem_usage		= 0; 
        F->w_string				("[TEXTURES]");
        F->w_string				("texture name=format,width,height,alpha,mem usage (Kb),area,pixel density,objects (name[count*area]),detail name,detail scale,bump name");
        for (u32 stt=sttFirst; stt<sttLast; stt++){   
            u32 cur_mem_usage	= 0; 
            float cur_area		= 0; 
            xr_string pref	= "[";
            pref				+= get_token_name(summary_texture_type_tokens,stt);
            pref				+= "]";
            F->w_string			(pref.c_str());
            for (TISetIt it=textures.begin(); it!=textures.end(); it++){
                STextureInfo* info= (STextureInfo*)(&(*it));
                if (info->type==stt){ 
                    cur_area	+= info->effective_area;
                    info->Export(F,cur_mem_usage);
                }
            }
            total_mem_usage		+= cur_mem_usage;
            sprintf				(tmp,"%s mem usage - %d Kb",pref.c_str(),cur_mem_usage);
            F->w_string			(tmp);
            sprintf				(tmp,"%s effective area - %3.2f m^2",pref.c_str(),cur_area);
            F->w_string			(tmp);
        }
        sprintf					(tmp,"Total mem usage - %d Kb",total_mem_usage);
        F->w_string				(tmp);
        // objects
        F->w_string				("");
        sprintf					(tmp,"[OBJECTS]");	F->w_string(tmp);
        for (OISetIt o_it=objects.begin(); o_it!=objects.end(); o_it++){
            SObjectInfo* info= (SObjectInfo*)(&(*o_it));
            info->Export		(F);
        }
        FS.w_close				(F);
        return 					true;
    }else{
        return 					false;
    }
}
bool SSceneSummary::OnWeightAfterEditClick(PropValue* sender, float& edit_val)
{
	if (sender->tag==0){
    	return edit_val<pm_colors[sender->tag+1].pm;
    }else if (sender->tag==pm_colors.size()-1){
    	return edit_val>pm_colors[sender->tag-1].pm;
    }else{
		return edit_val>pm_colors[sender->tag-1].pm && edit_val<pm_colors[sender->tag+1].pm;
    }
}
void SSceneSummary::FillProp(PropItemVec& items)
{
    // fill items
    ButtonValue* B =PHelper().CreateButton (items,"Common\\File","Export...",0);
    B->OnBtnClickEvent.bind(this,&SSceneSummary::OnFileClick);
    // fill items
    PHelper().CreateCaption(items,"Common\\Level Name",			Scene->m_LevelOp.m_FNLevelPath.c_str());
    PHelper().CreateCaption(items,"Geometry\\Bounding\\Min", 	shared_str().printf("{%3.2f, %3.2f, %3.2f}",VPUSH(bbox.min)));
    PHelper().CreateCaption(items,"Geometry\\Bounding\\Max", 	shared_str().printf("{%3.2f, %3.2f, %3.2f}",VPUSH(bbox.max)));
    PHelper().CreateCaption(items,"Geometry\\Mesh\\Total Faces",   	shared_str().printf("%d",face_cnt));
    PHelper().CreateCaption(items,"Geometry\\Mesh\\Total Vertices",	shared_str().printf("%d",vert_cnt));
    PHelper().CreateCaption(items,"Geometry\\MU\\Objects",	   	shared_str().printf("%d",mu_objects.size()));
    PHelper().CreateCaption(items,"Geometry\\MU\\References",   shared_str().printf("%d",object_mu_ref_cnt));
    PHelper().CreateCaption(items,"Geometry\\LOD\\Objects",		shared_str().printf("%d",lod_objects.size()));
    PHelper().CreateCaption(items,"Geometry\\LOD\\References",	shared_str().printf("%d",object_lod_ref_cnt));
    PHelper().CreateCaption(items,"Visibility\\HOM\\Faces",		shared_str().printf("%d",hom_face_cnt));
    PHelper().CreateCaption(items,"Visibility\\HOM\\Vertices",	shared_str().printf("%d",hom_vert_cnt));
    PHelper().CreateCaption(items,"Visibility\\Sectors",		shared_str().printf("%d",sector_cnt));
    PHelper().CreateCaption(items,"Visibility\\Portals",		shared_str().printf("%d",portal_cnt));
    PHelper().CreateCaption(items,"Lights\\Count",				shared_str().printf("%d",light_point_cnt+light_spot_cnt));
    PHelper().CreateCaption(items,"Lights\\By Type\\Point",		shared_str().printf("%d",light_point_cnt));
    PHelper().CreateCaption(items,"Lights\\By Type\\Spot",		shared_str().printf("%d",light_spot_cnt));
    PHelper().CreateCaption(items,"Lights\\By Usage\\Dynamic",	shared_str().printf("%d",light_dynamic_cnt));
    PHelper().CreateCaption(items,"Lights\\By Usage\\Static",	shared_str().printf("%d",light_static_cnt));
    PHelper().CreateCaption(items,"Lights\\By Usage\\Breakable",shared_str().printf("%d",light_breakable_cnt));
    PHelper().CreateCaption(items,"Lights\\By Usage\\Procedural",shared_str().printf("%d",light_procedural_cnt));
    PHelper().CreateCaption(items,"Glows\\Count",				shared_str().printf("%d",glow_cnt));
    // objects
    for (OISetIt o_it=objects.begin(); o_it!=objects.end(); o_it++){
    	SObjectInfo* info= (SObjectInfo*)(&(*o_it));
        info->FillProp		(items,"Objects");
    }
    // textures
    CaptionValue* total_count=PHelper().CreateCaption	(items,"Textures\\Count","");
    CaptionValue* total_mem	= PHelper().CreateCaption	(items,"Textures\\Memory Usage","");
    u32 total_mem_usage		= 0; 
    ButtonValue* BB			= PHelper().CreateButton	(items,"Textures\\Highlight Textures\\Command", "Pixel Density,Clear", ButtonValue::flFirstOnly);
    BB->OnBtnClickEvent.bind(this,&SSceneSummary::OnHighlightClick);
    for (PDVecIt pd_it=pm_colors.begin(); pd_it!=pm_colors.end(); pd_it++){
    	string128 tmp;		
        sprintf				(tmp,"Textures\\Highlight Textures\\Color Legend\\Item #%d",pd_it-pm_colors.begin());
    	PHelper().CreateColor(items,PrepareKey(tmp,"Color").c_str(),&pd_it->color);
    	FloatValue* V		= PHelper().CreateFloat(items,PrepareKey(tmp,"Weight (p/m)").c_str(),&pd_it->pm,0,1000000,1,0);
        V->OnAfterEditEvent.bind(this,&SSceneSummary::OnWeightAfterEditClick);
        V->tag				= pd_it-pm_colors.begin();
    }
    for (u32 stt=sttFirst; stt<sttLast; stt++){
        LPCSTR nm			= get_token_name(summary_texture_type_tokens,stt);
        if (nm&&nm[0]){
            u32 cur_mem_usage	= 0; 
            float cur_area		= 0; 
            shared_str pref		= PrepareKey("Textures",nm);
            CaptionValue* mem 	= PHelper().CreateCaption(items,PrepareKey(pref.c_str(),"Memory Usage").c_str(), "");
            CaptionValue* area 	= PHelper().CreateCaption(items,PrepareKey(pref.c_str(),"Effective Area").c_str(), "");
            for (TISetIt it=textures.begin(); it!=textures.end(); it++){
                STextureInfo* info= (STextureInfo*)(&(*it));
                if (info->type==stt){ 
                    cur_area	+= info->effective_area;
                    info->FillProp(items,pref.c_str(),cur_mem_usage);
                }
            }
            mem->ApplyValue		(shared_str().printf("%d Kb",iFloor(cur_mem_usage/1024)));
            area->ApplyValue 	(shared_str().printf("%3.2f m^2",cur_area));
            total_mem_usage		+= cur_mem_usage;
        }
    }
    total_count->ApplyValue	(shared_str().printf("%d",		textures.size()));
    total_mem->ApplyValue	(shared_str().printf("%d Kb",	iFloor(total_mem_usage/1024)));
	// sound
    PHelper().CreateCaption	(items,"Sounds\\Occluder\\Faces",		shared_str().printf("%d",snd_occ_face_cnt));
    PHelper().CreateCaption	(items,"Sounds\\Occluder\\Vertices",	shared_str().printf("%d",snd_occ_vert_cnt));
    PHelper().CreateCaption	(items,"Sounds\\Sources",				shared_str().printf("%d",sound_source_cnt));
    PHelper().CreateCaption	(items,"Sounds\\Waves\\Count",			shared_str().printf("%d",waves.size()));
    for (RStringSetIt w_it=waves.begin(); w_it!=waves.end(); w_it++)
        PHelper().CreateCaption(items,PrepareKey("Sounds\\Waves",w_it->c_str()),"-");
    // particles
    PHelper().CreateCaption	(items,"Particle System\\Sources",		shared_str().printf("%d",pe_static_cnt));
    PHelper().CreateCaption	(items,"Particle System\\Refs\\Count",	shared_str().printf("%d",pe_static.size()));
    for (RStringSetIt pe_it=pe_static.begin(); pe_it!=pe_static.end(); pe_it++)
        PHelper().CreateCaption(items,PrepareKey("Particle System\\Refs",pe_it->c_str()),"-");
}

SSceneSummary::SSceneSummary()
{
	Clear	();
    pm_colors.push_back	(SPixelDensityPair(0,0x80FF0000));
    pm_colors.push_back	(SPixelDensityPair(100,0x80FFFF00));
    pm_colors.push_back	(SPixelDensityPair(200,0x8000FF00));
    pm_colors.push_back	(SPixelDensityPair(500,0x8000FFFF));
    pm_colors.push_back	(SPixelDensityPair(1000,0x800000FF));
}

void SSceneSummary::Save(CInifile* I)
{
	I->w_u32			("summary_info","pm_count",s_summary.pm_colors.size());
    for (u32 idx=0; idx<s_summary.pm_colors.size(); ++idx){
    	string128 		tmp;
        sprintf			(tmp,"pm_item_%d_color",idx);
    	I->w_u32		("summary_info",tmp,s_summary.pm_colors[idx].color);
        sprintf			(tmp,"pm_item_%d_weight",idx);
    	I->w_float		("summary_info",tmp,s_summary.pm_colors[idx].pm);
    }
}

void SSceneSummary::Load(CInifile* I)
{
	if (I->section_exist("summary_info")&&I->line_exist("summary_info","pm_count")&&(5==I->r_u32("summary_info","pm_count"))){
        for (u32 idx=0; idx<s_summary.pm_colors.size(); ++idx){
            string128 	tmp;
            sprintf		(tmp,"pm_item_%d_color",idx);
		    s_summary.pm_colors[idx].color	= R_U32_SAFE("summary_info",tmp,s_summary.pm_colors[idx].color);
	        sprintf		(tmp,"pm_item_%d_weight",idx);
		    s_summary.pm_colors[idx].pm		= R_FLOAT_SAFE("summary_info",tmp,s_summary.pm_colors[idx].pm);
        }
    }
    std::sort			(s_summary.pm_colors.begin(),s_summary.pm_colors.end());
}

u32 SSceneSummary::SelectPMColor(float pm)
{
	VERIFY			(!s_summary.pm_colors.empty());
    u32 from;
    u32 to;
    float w			= 0.f;
    if (pm<s_summary.pm_colors[0].pm){
    	from		= s_summary.pm_colors[0].color;
	    to	 		= s_summary.pm_colors[0].color;
    }else if (pm>=s_summary.pm_colors[s_summary.pm_colors.size()-1].pm){
    	from		= s_summary.pm_colors.back().color;
	    to	 		= s_summary.pm_colors.back().color;
    }else{
	    u32 idx_clr	= 0;
        for (; idx_clr<s_summary.pm_colors.size()-1; ++idx_clr)
            if (pm>=s_summary.pm_colors[idx_clr].pm && pm<s_summary.pm_colors[idx_clr+1].pm) break;
        w			= (pm-s_summary.pm_colors[idx_clr].pm)/float(s_summary.pm_colors[idx_clr+1].pm);
        from		= s_summary.pm_colors[idx_clr+0].color;
        to			= s_summary.pm_colors[idx_clr+1].color;
    }
    float inv_w		= 1.f-w;
    return color_rgba	(color_get_R(from)*inv_w+color_get_R(to)*w, color_get_G(from)*inv_w+color_get_G(to)*w,
                         color_get_B(from)*inv_w+color_get_B(to)*w, color_get_A(from)*inv_w+color_get_A(to)*w);
}

void EScene::ClearSummaryInfo	()
{
	s_summary.Clear				();
}

void EScene::CollectSummaryInfo	()
{
    SceneToolsMapPairIt _I = m_SceneTools.begin();
    SceneToolsMapPairIt _E = m_SceneTools.end();
    for (; _I!=_E; _I++)
        if (_I->second)	_I->second->GetSummaryInfo(&s_summary);
    s_summary.Prepare				();
}

void EScene::ShowSummaryInfo		()
{
	PropItemVec items;
    // fill items
    s_summary.FillProp				(items);
    m_SummaryInfo->ShowProperties	();
	m_SummaryInfo->AssignItems		(items);
}

void EScene::ExportSummaryInfo	(LPCSTR f_name)
{
    s_summary.ExportSummaryInfo	(f_name);
}
//--------------------------------------------------------------------------------------------------

 
