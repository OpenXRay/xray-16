//----------------------------------------------------
// file: Light.cpp
//----------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "Log.h"
#include "Glow.h"
#include "xr_trims.h"
#include "bottombar.h"
#include "../../ecore/editor/D3DUtils.h"
#include "scene.h"
#include "ESceneGlowTools.h"

#define GLOW_VERSION	   				0x0012
//----------------------------------------------------
#define GLOW_CHUNK_VERSION				0xC411
#define GLOW_CHUNK_PARAMS				0xC413
#define GLOW_CHUNK_SHADER				0xC414
#define GLOW_CHUNK_TEXTURE				0xC415
#define GLOW_CHUNK_FLAGS				0xC416
//----------------------------------------------------

#define VIS_RADIUS 		0.25f

CGlow::CGlow(LPVOID data, LPCSTR name):CCustomObject(data,name){
	Construct(data);
}

void CGlow::Construct(LPVOID data){
	ClassID		= OBJCLASS_GLOW;
    m_GShader   = 0;
    m_fRadius	= 0.5f;
    m_bDefLoad	= false;
    m_Flags.zero();
    m_ShaderName= "effects\\glow";
}

CGlow::~CGlow()
{
	OnDeviceDestroy();
}

void CGlow::OnDeviceCreate()
{
	if (m_bDefLoad) return;
	// создать заново shaders
	if (m_TexName.size()&&m_ShaderName.size()) m_GShader.create(*m_ShaderName,*m_TexName);
	m_bDefLoad = true;
}

void CGlow::OnDeviceDestroy()
{
	m_bDefLoad = false;
	// удалить shaders
	m_GShader.destroy();
}

void CGlow::ShaderChange(PropValue* value)
{
	OnDeviceDestroy();
}

bool CGlow::GetBox( Fbox& box ) const
{
	box.set( PPosition, PPosition );
	box.min.sub(m_fRadius);
	box.max.add(m_fRadius);
	return true;
}

void CGlow::Render(int priority, bool strictB2F)
{
    if ((1==priority)&&(true==strictB2F))
    {
    	if (!m_bDefLoad) OnDeviceCreate();
        ESceneGlowTool* gt 		= dynamic_cast<ESceneGlowTool*>(ParentTool);
        VERIFY					(gt);
        RCache.set_xform_world	(Fidentity);

        if (gt->m_Flags.is(ESceneGlowTool::flTestVisibility))
        { 
            Fvector D;
            D.sub(EDevice.vCameraPosition,PPosition);
            float dist 	= D.normalize_magn();
            if (!Scene->RayPickObject(dist,PPosition,D,OBJCLASS_SCENEOBJECT,0,0)){
                if (m_GShader){	EDevice.SetShader(m_GShader);
                }else{			EDevice.SetShader(EDevice.m_WireShader);}
                m_RenderSprite.Render(PPosition,m_fRadius,m_Flags.is(gfFixedSize));
                DU_impl.DrawRomboid(PPosition, VIS_RADIUS, 0x00FF8507);
            }else{
                // рендерим bounding sphere
                EDevice.SetShader(EDevice.m_WireShader);
                DU_impl.DrawRomboid(PPosition, VIS_RADIUS, 0x00FF8507);
            }
        }else{
            if (m_GShader){	EDevice.SetShader(m_GShader);
            }else{			EDevice.SetShader(EDevice.m_WireShader);}
            m_RenderSprite.Render(PPosition,m_fRadius,m_Flags.is(gfFixedSize));
        }
        if( Selected() ){
            Fbox bb; GetBox(bb);
            u32 clr = 0xFFFFFFFF;
            EDevice.SetShader(EDevice.m_WireShader);
            DU_impl.DrawSelectionBoxB(bb,&clr);
            if (gt->m_Flags.is(ESceneGlowTool::flDrawCross))
            {
            	Fvector sz; bb.getradius(sz);
        		DU_impl.DrawCross(PPosition,sz.x,sz.y,sz.z, sz.x,sz.y,sz.z,0xFFFFFFFF,false);
            }
        }
    }
}

bool CGlow::FrustumPick(const CFrustum& frustum)
{
    return (frustum.testSphere_dirty(PPosition,m_fRadius))?true:false;
}

bool CGlow::RayPick(float& distance, const Fvector& start, const Fvector& direction, SRayPickInfo* pinf)
{
	Fvector ray2;
	ray2.sub( PPosition, start );

    float d = ray2.dotproduct(direction);
    if( d > 0  ){
        float d2 = ray2.magnitude();
        if( ((d2*d2-d*d) < (m_fRadius*m_fRadius)) && (d>m_fRadius) ){
        	if (d<distance){
	            distance = d;
    	        return true;
            }
        }
    }
	return false;
}

bool CGlow::LoadLTX(CInifile& ini, LPCSTR sect_name)
{
	u32 version = ini.r_u32(sect_name, "version");

    if(version!=GLOW_VERSION)
    {
        ELog.DlgMsg( mtError, "CGlow: Unsupported version.");
        return false;
    }

	CCustomObject::LoadLTX(ini, sect_name);

   	m_ShaderName		= ini.r_string (sect_name, "shader_name");

	m_TexName			= ini.r_string	(sect_name, "texture_name");

	m_fRadius  			= ini.r_float	(sect_name, "radius");

    m_Flags.assign		(ini.r_u32(sect_name, "flags"));

    return true;
}

void CGlow::SaveLTX(CInifile& ini, LPCSTR sect_name)
{
	CCustomObject::SaveLTX(ini, sect_name);

	ini.w_u16		(sect_name, "version", GLOW_VERSION);

	ini.w_float   	(sect_name, "radius", m_fRadius);

    ini.w_string 	(sect_name, "shader_name", m_ShaderName.c_str());

	ini.w_string	(sect_name, "texture_name", m_TexName.c_str());

	ini.w_u16		(sect_name, "flags", m_Flags.get());
}

bool CGlow::LoadStream(IReader& F)
{
	u16 version = 0;

    R_ASSERT(F.r_chunk(GLOW_CHUNK_VERSION,&version));
    if((version!=0x0011)&&(version!=GLOW_VERSION)){
        ELog.DlgMsg( mtError, "CGlow: Unsupported version.");
        return false;
    }

	CCustomObject::LoadStream(F);

    if (F.find_chunk(GLOW_CHUNK_SHADER)){
    	F.r_stringZ (m_ShaderName);
    }

    R_ASSERT(F.find_chunk(GLOW_CHUNK_TEXTURE));
	F.r_stringZ	(m_TexName);

    R_ASSERT(F.find_chunk(GLOW_CHUNK_PARAMS));
	m_fRadius  		= F.r_float();
	if (version==0x0011){
		F.r_fvector3	(FPosition);
        UpdateTransform();
    }

    if (F.find_chunk(GLOW_CHUNK_FLAGS))
    	m_Flags.assign	(F.r_u16());

    return true;
}

void CGlow::SaveStream(IWriter& F)
{
	CCustomObject::SaveStream(F);

	F.open_chunk	(GLOW_CHUNK_VERSION);
	F.w_u16			(GLOW_VERSION);
	F.close_chunk	();

	F.open_chunk	(GLOW_CHUNK_PARAMS);
	F.w_float   		(m_fRadius);
	F.close_chunk	();

    F.open_chunk	(GLOW_CHUNK_SHADER);
    F.w_stringZ 	(m_ShaderName);
    F.close_chunk	();

	F.open_chunk	(GLOW_CHUNK_TEXTURE);
	F.w_stringZ		(m_TexName);
	F.close_chunk	();

	F.open_chunk	(GLOW_CHUNK_FLAGS);
	F.w_u16			(m_Flags.get());
	F.close_chunk	();
}
//----------------------------------------------------

void CGlow::FillProp(LPCSTR pref, PropItemVec& items)
{
	inherited::FillProp(pref, items);
    PropValue* V=0;
    V=PHelper().CreateChoose	(items,PrepareKey(pref,"Texture"), 	&m_TexName,		smTexture);	V->OnChangeEvent.bind(this,&CGlow::ShaderChange);
    V=PHelper().CreateChoose	(items,PrepareKey(pref,"Shader"),	&m_ShaderName,	smEShader);	V->OnChangeEvent.bind(this,&CGlow::ShaderChange);
    PHelper().CreateFloat		(items,PrepareKey(pref,"Radius"),	&m_fRadius,		0.01f,10000.f);
//.	PHelper().CreateFlag<Flags8>(items,PHelper().PrepareKey(pref,"Fixed size"),	&m_Flags, 		gfFixedSize);
}
//----------------------------------------------------

bool CGlow::GetSummaryInfo(SSceneSummary* inf)
{
	inherited::GetSummaryInfo	(inf);
	if (m_TexName.size()) 	inf->AppendTexture(ChangeFileExt(*m_TexName,"").LowerCase().c_str(),SSceneSummary::sttGlow,0,0,"$GLOW$");
	inf->glow_cnt++;
	return true;
}

