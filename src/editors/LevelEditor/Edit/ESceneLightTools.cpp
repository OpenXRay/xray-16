#include "stdafx.h"
#pragma hdrstop

#include "ESceneLightTools.h"
#include "IGame_Persistent.h"
#include "../ECore/Editor/d3dutils.h"
#include "communicate.h"
#include "../ECore/Editor/ui_main.h"
#include "../xrEProps/TextForm.h"
#include "ELight.h"

ESceneLightTool::ESceneLightTool():ESceneCustomOTool(OBJCLASS_LIGHT)
{
	Clear				();
    m_Flags.zero		();
}
//------------------------------------------------------------------------------

ESceneLightTool::~ESceneLightTool()
{
}
//------------------------------------------------------------------------------

void ESceneLightTool::Clear(bool bSpecific)
{
	inherited::Clear(bSpecific);

    m_SunShadowDir.set	(deg2rad(-25.f),deg2rad(292.f));
    
	lcontrol_last_idx	= 0;
    lcontrols.clear		();
    AppendLightControl	(LCONTROL_STATIC);
    AppendLightControl	(LCONTROL_HEMI);
    AppendLightControl	(LCONTROL_SUN);
    m_HemiControl		= FindLightControl(LCONTROL_HEMI)->id;
    m_Flags.zero		();
}
//------------------------------------------------------------------------------

void ESceneLightTool::SelectLightsForObject(CCustomObject* obj)
{
    for (u32 i=0; i<frame_light.size(); i++){
        CLight* l = frame_light[i];
//        if (obj->IsDynamic()&&!l->m_Flags.is(CLight::flAffectDynamic)) continue;
//        if (!obj->IsDynamic()&&!l->m_Flags.is(CLight::flAffectStatic)) continue;
        Fbox bb; 	obj->GetBox(bb);
        Fvector C; 	float R; bb.getsphere(C,R);
        float d 	= C.distance_to(l->PPosition) - l->m_Range - R;
        EDevice.LightEnable(i,(d<0));
    }
}

void ESceneLightTool::AppendFrameLight(CLight* src)
{
    Flight L;
    ZeroMemory			(&L, sizeof(Flight));
    L.type				= src->m_Type;
    L.diffuse.mul_rgb	(src->m_Color,src->m_Brightness);
    L.specular.set		(L.diffuse);
    L.position.set		(src->PPosition);
    Fvector dir;    	dir.setHP(src->PRotation.y,src->PRotation.x);
    L.direction.set		(dir);
    L.range				= src->m_Range;
    L.attenuation0		= src->m_Attenuation0+EPS_S;
    L.attenuation1		= src->m_Attenuation1;
    L.attenuation2		= src->m_Attenuation2;
    L.phi				= src->m_Cone;
    L.falloff			= 1.f;
    EDevice.SetLight		(frame_light.size(),L);
    frame_light.push_back(src);
}

void ESceneLightTool::BeforeRender()
{
    if (psDeviceFlags.is(rsLighting)){
        int l_cnt		= 0;
        // set scene lights
        for(ObjectIt _F = m_Objects.begin();_F!=m_Objects.end();_F++){
            CLight* l 		= (CLight*)(*_F);
            l_cnt++;
            if (l->Visible()&&l->m_UseInD3D&&l->m_Flags.is_any(ELight::flAffectDynamic|ELight::flAffectStatic))
                if (::Render->ViewBase.testSphere_dirty(l->PPosition,l->m_Range))
                	AppendFrameLight(l);
        }
    	// set sun
		if (m_Flags.is(flShowSun))
        {
            Flight L;
            Fvector C;
//            if (psDeviceFlags.is(rsEnvironment)){
//	            C			= g_pGamePersistent->Environment().CurrentEnv->sun_color;
//            }else{
            	C.set		(1.f,1.f,1.f);
//            }
            L.direction.setHP(m_SunShadowDir.y,m_SunShadowDir.x);
            L.diffuse.set	(C.x,C.y,C.z,1.f);
            L.ambient.set	(0.f,0.f,0.f,0.f);
            L.specular.set	(C.x,C.y,C.z,1.f);
            L.type			= D3DLIGHT_DIRECTIONAL;
            EDevice.SetLight	(frame_light.size(),L);
            EDevice.LightEnable(frame_light.size(),TRUE);
        }
		// ambient
//        if (psDeviceFlags.is(rsEnvironment)){
//	        Fvector& V		= g_pGamePersistent->Environment().CurrentEnv->ambient;
//            Fcolor C;		C.set(V.x,V.y,V.z,1.f);
//            EDevice.SetRS	(D3DRS_AMBIENT,C.get());
//        }else				
        	EDevice.SetRS(D3DRS_AMBIENT,0x00000000);
        
        EDevice.Statistic->dwTotalLight 	= l_cnt;
        EDevice.Statistic->dwLightInScene = frame_light.size();
    }
}
//------------------------------------------------------------------------------

void ESceneLightTool::AfterRender()
{
    if (m_Flags.is(flShowSun))
        EDevice.LightEnable(frame_light.size(),FALSE); // sun - last light!
    for (u32 i=0; i<frame_light.size(); i++)
		EDevice.LightEnable(i,FALSE);
    frame_light.clear();
}
//------------------------------------------------------------------------------

#define VIS_RADIUS 		0.03f
void  ESceneLightTool::OnRender(int priority, bool strictB2F)
{
	inherited::OnRender(priority, strictB2F);
    if (m_Flags.is(flShowSun)){
        if ((true==strictB2F)&&(1==priority)){
            EDevice.SetShader		(EDevice.m_WireShader);
            RCache.set_xform_world	(Fidentity);
            Fvector dir;
            dir.setHP(m_SunShadowDir.y,m_SunShadowDir.x);
            Fvector p;
            float fd				= UI->ZFar()*0.95f;
            p.mad					(EDevice.vCameraPosition,dir,-fd);
            DU_impl.DrawPointLight		( p ,VIS_RADIUS*fd, 0x00FFE020);
            DU_impl.DrawLineSphere		( p, VIS_RADIUS*fd*0.3f, 0x00FF3000, false );
        }
    }
}
//------------------------------------------------------------------------------

void ESceneLightTool::OnControlAppendClick(ButtonValue* sender, bool& bDataModified, bool& bSafe)
{
	AppendLightControl(GenLightControlName().c_str());
    ExecCommand(COMMAND_UPDATE_PROPERTIES);
    bDataModified = true;
}
//------------------------------------------------------------------------------

void ESceneLightTool::OnControlRenameRemoveClick(ButtonValue* V, bool& bDataModified, bool& bSafe)
{
    AnsiString item_name = V->Owner()->Item()->Text;
    switch (V->btn_num){
    case 0:{ 
    	AnsiString new_name=item_name;
    	if (TfrmText::RunEditor(new_name,"Control name")){
        	if (FindLightControl(new_name.c_str())){
            	ELog.DlgMsg(mtError,"Duplicate name found.");
            }else if (new_name.IsEmpty()||new_name.Pos("\\")){
            	ELog.DlgMsg(mtError,"Invalid control name.");
			}else{
            	RTokenVecIt it	= FindLightControlIt(item_name.c_str());
                it->rename 		(new_name.c_str());
            }
        }
    }break;
    case 1: RemoveLightControl(item_name.c_str());	break;
	}
    ExecCommand(COMMAND_UPDATE_PROPERTIES);
    bDataModified = true;
}
//------------------------------------------------------------------------------
void ESceneLightTool::FillProp(LPCSTR pref, PropItemVec& items)
{
    ButtonValue*	B 	= 0;
    // hemisphere
//.	PHelper().CreateRToken32(items, PrepareKey(pref,"Common\\Hemisphere\\Light Control"),	&m_HemiControl, 	&*lcontrols.begin(), lcontrols.size());
    
    // sun
    PHelper().CreateFlag32	(items, PrepareKey(pref,"Common\\Sun Shadow\\Visible"),			&m_Flags,			flShowSun);
    PHelper().CreateAngle	(items,	PrepareKey(pref,"Common\\Sun Shadow\\Altitude"),			&m_SunShadowDir.x,	-PI_DIV_2,0);
    PHelper().CreateAngle	(items,	PrepareKey(pref,"Common\\Sun Shadow\\Longitude"),		&m_SunShadowDir.y,	0,PI_MUL_2);
    // light controls
    PHelper().CreateFlag32	(items, PrepareKey(pref,"Common\\Controls\\Draw Name"),			&m_Flags,			flShowControlName);
    PHelper().CreateCaption	(items,PrepareKey(pref,"Common\\Controls\\Count"),				shared_str().printf("%d",lcontrols.size()));
//	B=PHelper().CreateButton(items,PHelper().PrepareKey(pref,"Common\\Controls\\Edit"),	"Append",	ButtonValue::flFirstOnly);
//	B->OnBtnClickEvent	= OnControlAppendClick;
	RTokenVecIt		_I 	= lcontrols.begin();
    RTokenVecIt		_E 	= lcontrols.end();
    for (;_I!=_E; _I++){
    	if (_I->equal(LCONTROL_HEMI)||_I->equal(LCONTROL_STATIC)||_I->equal(LCONTROL_SUN)){
		    PHelper().CreateCaption(items,	PrepareKey(pref,"Common\\Controls\\System",*_I->name),"");
        }else{
		    B=PHelper().CreateButton(items,	PrepareKey(pref,"Common\\Controls\\User",*_I->name),"Rename,Remove",ButtonValue::flFirstOnly);
            B->OnBtnClickEvent.bind		(this,&ESceneLightTool::OnControlRenameRemoveClick);
        }
    }                              
	inherited::FillProp(pref, items);
}
//------------------------------------------------------------------------------

AnsiString ESceneLightTool::GenLightControlName()
{
	AnsiString name;
    int idx=0;
    do{
    	name.sprintf("control_%02d",idx++);
    }while (FindLightControl(name.c_str()));
    return name;
}
//------------------------------------------------------------------------------

xr_rtoken* ESceneLightTool::FindLightControl(int id)
{
	RTokenVecIt		_I 	= lcontrols.begin();
    RTokenVecIt		_E 	= lcontrols.end();
    for (;_I!=_E; _I++)
    	if (_I->id==id) return _I;
    return 0;
}
//------------------------------------------------------------------------------

RTokenVecIt ESceneLightTool::FindLightControlIt(LPCSTR name)
{
	RTokenVecIt		_I 	= lcontrols.begin();
    RTokenVecIt		_E 	= lcontrols.end();
    for (;_I!=_E; _I++)
    	if (_I->equal(name)) return _I;
    return lcontrols.end();
}
//------------------------------------------------------------------------------

void ESceneLightTool::AppendLightControl(LPCSTR nm, u32* idx)
{
	AnsiString name = nm; _Trim(name);
    if (name.IsEmpty()) return;
	if (FindLightControl(name.c_str())) return;
	lcontrols.push_back	(xr_rtoken(name.c_str(),idx?*idx:lcontrol_last_idx++));
}
//------------------------------------------------------------------------------

void ESceneLightTool::RemoveLightControl(LPCSTR name)
{
	RTokenVecIt it	= FindLightControlIt(name);
    if (it!=lcontrols.end()) lcontrols.erase(it);
}
//------------------------------------------------------------------------------

bool ESceneLightTool::Validate(bool full_test)
{
	if (!inherited::Validate(full_test)) return false;
	bool bRes = !m_Objects.empty();
	for (ObjectIt it=m_Objects.begin(); it!=m_Objects.end(); it++){
    	CLight* L = dynamic_cast<CLight*>(*it);
    	if (!L->GetLControlName()){
        	bRes=false;
            ELog.Msg(mtError,"%s: '%s' - Invalid light control.",ClassDesc(),L->Name);
        }
    }
    return bRes;
}
//------------------------------------------------------------------------------


#include "frameLight.h"
#include "UI_LevelTools.h"

void ESceneLightTool::CreateControls()
{
	inherited::CreateDefaultControls(estDefault);
	// frame
    pFrame 			= xr_new<TfraLight>((TComponent*)0);
}
//----------------------------------------------------
 
void ESceneLightTool::RemoveControls()
{
	inherited::RemoveControls();
}
//----------------------------------------------------

CCustomObject* ESceneLightTool::CreateObject(LPVOID data, LPCSTR name)
{
	CCustomObject* O	= xr_new<CLight>(data,name);
    O->ParentTool		= this;
    return O;
}
//----------------------------------------------------



/*
	m_D3D.direction.setHP(PRotation.y,PRotation.x);
	if (D3DLIGHT_DIRECTIONAL==m_D3D.type) m_LensFlare.Update(m_D3D.direction, m_D3D.diffuse);

//render
        case D3DLIGHT_DIRECTIONAL:
            if (Selected()) DU.DrawDirectionalLight( m_D3D.position, m_D3D.direction, VIS_RADIUS, DIR_SELRANGE, clr );
            else			DU.DrawDirectionalLight( m_D3D.position, m_D3D.direction, VIS_RADIUS, DIR_RANGE, clr );
        break;
else if ((3==priority)&&(true==strictB2F)){
		if (D3DLIGHT_DIRECTIONAL==m_D3D.type) m_LensFlare.Render();
    }
// update
	if (D3DLIGHT_DIRECTIONAL==m_D3D.type){
    	m_LensFlare.Update(m_D3D.direction, m_D3D.diffuse);
	    m_LensFlare.DeleteShaders();
    	m_LensFlare.CreateShaders();
    }
// load
	m_LensFlare.Load(F);
// save
	if (D3DLIGHT_DIRECTIONAL==m_D3D.type) m_LensFlare.Save(F);

    
void CLight:xr_token sun_quality[]={
	{ "Low",			1	},
	{ "Normal",			2	},
	{ "Final",			3	},
	{ 0,				0	}
};

:FillSunProp(LPCSTR pref, PropItemVec& items)
{
	CEditFlare& F 			= m_LensFlare;
    PropValue* prop			= 0;
    PHelper().CreateToken		(items, PHelper().PrepareKey(pref,"Sun\\Quality"),				&m_SunQuality,			sun_quality);
    
    PHelper().CreateFlag32	(items, PHelper().PrepareKey(pref,"Sun\\Source\\Enabled"),		&F.m_Flags,				CEditFlare::flSource);
    PHelper().CreateFloat		(items, PHelper().PrepareKey(pref,"Sun\\Source\\Radius"),			&F.m_Source.fRadius,	0.f,10.f);
    prop 					= PHelper().CreateTexture	(items, PHelper().PrepareKey(pref,"Sun\\Source\\Texture"),		F.m_Source.texture,		sizeof(F.m_Source.texture));
	prop->OnChangeEvent		= OnNeedUpdate;

    PHelper().CreateFlag32	(items, PHelper().PrepareKey(pref,"Sun\\Gradient\\Enabled"),		&F.m_Flags,				CEditFlare::flGradient);
    PHelper().CreateFloat		(items, PHelper().PrepareKey(pref,"Sun\\Gradient\\Radius"),		&F.m_Gradient.fRadius,	0.f,100.f);
    PHelper().CreateFloat		(items, PHelper().PrepareKey(pref,"Sun\\Gradient\\Opacity"),		&F.m_Gradient.fOpacity,	0.f,1.f);
	prop					= PHelper().CreateTexture	(items, PHelper().PrepareKey(pref,"Sun\\Gradient\\Texture"),	F.m_Gradient.texture,	sizeof(F.m_Gradient.texture));
	prop->OnChangeEvent		= OnNeedUpdate;

    PHelper().CreateFlag32	(items, PHelper().PrepareKey(pref,"Sun\\Flares\\Enabled"),		&F.m_Flags,				CEditFlare::flFlare);
	for (CEditFlare::FlareIt it=F.m_Flares.begin(); it!=F.m_Flares.end(); it++){
		AnsiString nm; nm.sprintf("%s\\Sun\\Flares\\Flare %d",pref,it-F.m_Flares.begin());
		PHelper().CreateFloat	(items, PHelper().PrepareKey(nm.c_str(),"Radius"), 	&it->fRadius,  	0.f,10.f);
        PHelper().CreateFloat	(items, PHelper().PrepareKey(nm.c_str(),"Opacity"),	&it->fOpacity,	0.f,1.f);
        PHelper().CreateFloat	(items, PHelper().PrepareKey(nm.c_str(),"Position"),	&it->fPosition,	-10.f,10.f);
        prop				= PHelper().CreateTexture	(items, PHelper().PrepareKey(nm.c_str(),"Texture"),	it->texture,	sizeof(it->texture));
        prop->OnChangeEvent	= OnNeedUpdate;
	}
}
//----------------------------------------------------
void CLight::OnDeviceCreate()
{
	if (D3DLIGHT_DIRECTIONAL==m_D3D.type) m_LensFlare.DDLoad();
}
void CLight::OnDeviceDestroy()
{
//	if (D3DLIGHT_DIRECTIONAL==m_D3D.type)
    m_LensFlare.DDUnload();
}
//----------------------------------------------------

//----------------------------------------------------
// Edit Flare
//----------------------------------------------------
CEditFlare::CEditFlare()
{
    m_Flags.set(flFlare|flSource|flGradient,TRUE);
	// flares
    m_Flares.resize		(6);
    FlareIt it=m_Flares.begin();
	it->fRadius=0.08f; it->fOpacity=0.18f; it->fPosition=1.3f; strcpy(it->texture,"fx\\fx_flare1"); it++;
	it->fRadius=0.12f; it->fOpacity=0.12f; it->fPosition=1.0f; strcpy(it->texture,"fx\\fx_flare2"); it++;
	it->fRadius=0.04f; it->fOpacity=0.30f; it->fPosition=0.5f; strcpy(it->texture,"fx\\fx_flare2"); it++;
	it->fRadius=0.08f; it->fOpacity=0.24f; it->fPosition=-0.3f; strcpy(it->texture,"fx\\fx_flare2"); it++;
	it->fRadius=0.12f; it->fOpacity=0.12f; it->fPosition=-0.6f; strcpy(it->texture,"fx\\fx_flare3"); it++;
	it->fRadius=0.30f; it->fOpacity=0.12f; it->fPosition=-1.0f; strcpy(it->texture,"fx\\fx_flare1"); it++;
	// source
    strcpy(m_Source.texture,"fx\\fx_sun");
    m_Source.fRadius 	= 0.15f;
    // gradient
    strcpy(m_Gradient.texture,"fx\\fx_gradient");
    m_Gradient.fOpacity = 0.9f;
    m_Gradient.fRadius 	= 4.f;
}

void CEditFlare::Load(IReader& F){
	if (!F.find_chunk(FLARE_CHUNK_FLAG)) return;

    R_ASSERT(F.find_chunk(FLARE_CHUNK_FLAG));
    F.r				(&m_Flags.flags,sizeof(m_Flags));

    R_ASSERT(F.find_chunk(FLARE_CHUNK_SOURCE));
    F.r_stringZ		(m_Source.texture);
    m_Source.fRadius= F.r_float();

    if (F.find_chunk(FLARE_CHUNK_GRADIENT2)){
	    F.r_stringZ	(m_Gradient.texture);
	    m_Gradient.fOpacity = F.r_float();
	    m_Gradient.fRadius  = F.r_float();
    }else{
		R_ASSERT(F.find_chunk(FLARE_CHUNK_GRADIENT));
	    m_Gradient.fOpacity = F.r_float();
    }

    // flares
    if (F.find_chunk(FLARE_CHUNK_FLARES2)){
	    DeleteShaders();
	    u32 deFCnt	= F.r_u32(); VERIFY(deFCnt==6);
	   	F.r				(m_Flares.begin(),m_Flares.size()*sizeof(SFlare));
    	for (FlareIt it=m_Flares.begin(); it!=m_Flares.end(); it++) it->hShader._clear();
    	CreateShaders();
    }
}
//----------------------------------------------------

void CEditFlare::Save(IWriter& F)
{
	F.open_chunk	(FLARE_CHUNK_FLAG);
    F.w				(&m_Flags.flags,sizeof(m_Flags));
	F.close_chunk	();

	F.open_chunk	(FLARE_CHUNK_SOURCE);
    F.w_stringZ		(m_Source.texture);
    F.w_float		(m_Source.fRadius);
	F.close_chunk	();

	F.open_chunk	(FLARE_CHUNK_GRADIENT2);
    F.w_stringZ		(m_Gradient.texture);
    F.w_float		(m_Gradient.fOpacity);
    F.w_float		(m_Gradient.fRadius);
	F.close_chunk	();

	F.open_chunk	(FLARE_CHUNK_FLARES2);
    F.w_u32			(m_Flares.size());
    F.w				(m_Flares.begin(),m_Flares.size()*sizeof(SFlare));
	F.close_chunk	();
}
//----------------------------------------------------

void CEditFlare::Render()
{
	CLensFlare::Render(m_Flags.is(flSource),m_Flags.is(flFlare),m_Flags.is(flGradient));
}
//----------------------------------------------------

void CEditFlare::DeleteShaders()
{
    CLensFlare::DDUnload();
}

void CEditFlare::CreateShaders()
{
    CLensFlare::DDLoad();
}
//----------------------------------------------------


	AnsiString suns;
	for (ObjectIt it=lst.begin(); it!=lst.end(); it++){
		switch ((*it)->ClassID){
        case OBJCLASS_LIGHT:{
            CLight *l = (CLight *)(*it);
            if (l->m_D3D.type==D3DLIGHT_DIRECTIONAL){
	            if (suns.Length()) suns += ", ";
    	        suns += l->Name;
           	    pIni->w_fcolor	(l->Name, "sun_color", 			l->m_D3D.diffuse);
               	pIni->w_fvector3(l->Name, "sun_dir", 			l->m_D3D.direction);
               	pIni->w_string	(l->Name, "gradient", 			l->m_LensFlare.m_Flags.is(CLensFlare::flGradient)?"on":"off");
               	pIni->w_string	(l->Name, "source", 			l->m_LensFlare.m_Flags.is(CLensFlare::flSource)?"on":"off");
               	pIni->w_string	(l->Name, "flares", 			l->m_LensFlare.m_Flags.is(CLensFlare::flFlare)?"on":"off");
                pIni->w_float	(l->Name, "gradient_opacity", 	l->m_LensFlare.m_Gradient.fOpacity);
                pIni->w_string	(l->Name, "gradient_texture", 	l->m_LensFlare.m_Gradient.texture);
                pIni->w_float	(l->Name, "gradient_radius", 	l->m_LensFlare.m_Gradient.fRadius);
                pIni->w_string	(l->Name, "source_texture", 	l->m_LensFlare.m_Source.texture);
                pIni->w_float	(l->Name, "source_radius", 	l->m_LensFlare.m_Source.fRadius);
                AnsiString FT=""; AnsiString FR=""; AnsiString FO=""; AnsiString FP="";
                AnsiString T="";
                int i=l->m_LensFlare.m_Flares.size();
                for (CEditFlare::FlareIt it = l->m_LensFlare.m_Flares.begin(); it!=l->m_LensFlare.m_Flares.end(); it++,i--){
                	FT += it->texture;
                	T.sprintf("%.3f",it->fRadius); 	FR += T;
                	T.sprintf("%.3f",it->fOpacity);	FO += T;
                	T.sprintf("%.3f",it->fPosition);FP += T;
                    if (i>1){FT+=","; FR+=","; FO+=","; FP+=",";}
                }
               	pIni->w_string	(l->Name, "flare_textures",	FT.c_str());
               	pIni->w_string	(l->Name, "flare_radius",	FR.c_str());
               	pIni->w_string	(l->Name, "flare_opacity",	FO.c_str());
               	pIni->w_string	(l->Name, "flare_position",	FP.c_str());
            }
        }break;
        case OBJCLASS_GROUP:{
            CGroupObject* group = (CGroupObject*)(*it);
            bResult = ParseLTX(pIni,group->GetObjects(),group->Name);
        }break;
        }
        if (!bResult) return FALSE;
        if (suns.Length()) pIni->w_string("environment", "suns", suns.c_str());
    }
*/
