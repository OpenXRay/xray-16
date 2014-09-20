//----------------------------------------------------
// file: ELight.cpp
//----------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "ELight.h"
#include "LightAnimLibrary.h"
#include "escenelighttools.h"

static const u32 LIGHT_VERSION   			= 0x0011;
//----------------------------------------------------
enum{
    LIGHT_CHUNK_VERSION			= 0xB411,
    LIGHT_CHUNK_FLAG			= 0xB413,
    LIGHT_CHUNK_BRIGHTNESS		= 0xB425,
    LIGHT_CHUNK_D3D_PARAMS     	= 0xB435,
    LIGHT_CHUNK_USE_IN_D3D		= 0xB436,
    LIGHT_CHUNK_ROTATE			= 0xB437,
    LIGHT_CHUNK_ANIMREF			= 0xB438,
    LIGHT_CHUNK_FALLOFF_TEXTURE	= 0xB439,
    LIGHT_CHUNK_FUZZY_DATA		= 0xB440,
    LIGHT_CHUNK_LCONTROL		= 0xB441,
    LIGHT_CHUNK_PARAMS     		= 0xB442,
};
//----------------------------------------------------

void CLight::SFuzzyData::SaveLTX(CInifile& ini, LPCSTR sect_name)
{
    ini.w_u8		(sect_name, "fuzzy_shape_type", 	m_ShapeType);
    ini.w_float		(sect_name, "fuzzy_sphere_radius",  m_SphereRadius);
    ini.w_fvector3	(sect_name,  "fuzzy_box_dim", 		m_BoxDimension);
    ini.w_u32		(sect_name,  "fuzzy_point_count", 	m_PointCount);

    string128		buff;
    for(u32 idx=0; idx<m_PointCount; ++idx)
    {
    	sprintf			(buff,"fuzzy_point_%d", idx);
        ini.w_fvector3	(sect_name, buff, m_Positions[idx]);
    }
}

void CLight::SFuzzyData::LoadLTX(CInifile& ini, LPCSTR sect_name)
{
    m_ShapeType		= ini.r_u8			(sect_name, "fuzzy_shape_type");
    m_SphereRadius	= ini.r_float		(sect_name, "fuzzy_sphere_radius");
    m_BoxDimension	= ini.r_fvector3	(sect_name,  "fuzzy_box_dim");
    m_PointCount	= ini.r_u32			(sect_name,  "fuzzy_point_count");

    string128					buff;
    m_Positions.clear			();
    for(u32 idx=0; idx<m_PointCount; ++idx)
    {
    	sprintf					(buff,"fuzzy_point_%d", idx);
        Fvector p				= ini.r_fvector3	(sect_name, buff);
        m_Positions.push_back 	(p);
    }
}

void CLight::SFuzzyData::SaveStream(IWriter& F)
{
    F.w_u8		(m_ShapeType);
    F.w_float	(m_SphereRadius);
    F.w_fvector3(m_BoxDimension);
    F.w_s16		(m_PointCount);
    F.w			(&*m_Positions.begin(),sizeof(Fvector)*m_PointCount);
}

void CLight::SFuzzyData::LoadStream(IReader& F)
{
    m_ShapeType		= (EShapeType)F.r_u8();
    m_SphereRadius	= F.r_float();
    F.r_fvector3	(m_BoxDimension);
    m_PointCount	= F.r_s16();
    m_Positions.resize(m_PointCount);
    F.r				(&*m_Positions.begin(),sizeof(Fvector)*m_PointCount);
}

bool CLight::LoadLTX(CInifile& ini, LPCSTR sect_name)
{
	u32 version = ini.r_u32(sect_name, "version");

    if(version!=LIGHT_VERSION)
    {
        ELog.DlgMsg( mtError, "CLight: Unsupported version.");
        return false;
    }

	CCustomObject::LoadLTX	(ini, sect_name);

    m_Type			= (ELight::EType)(ini.r_u32(sect_name, "type"));
    m_Color			= ini.r_fcolor		(sect_name, "color");
    m_Brightness	= ini.r_float	    (sect_name, "brightness");
    m_Range			= ini.r_float	    (sect_name, "range");
    m_Attenuation0	= ini.r_float	    (sect_name, "attenuation0");
    m_Attenuation1	= ini.r_float	    (sect_name, "attenuation1");
    m_Attenuation2	= ini.r_float	    (sect_name, "attenuation2");
    m_Cone			= ini.r_float	    (sect_name, "cone");
    m_VirtualSize	= ini.r_float	    (sect_name, "virtual_size");

    m_UseInD3D		= ini.r_bool		(sect_name, "use_in_d3d");
    m_Flags.assign	(ini.r_u32			(sect_name, "light_flags"));
    m_LControl		= ini.r_u32			(sect_name, "light_control");

	LPCSTR anm		= ini.r_string		(sect_name,"anim_ref_name");
    if(anm)
    {
        m_pAnimRef	= LALib.FindItem(anm);
        if (!m_pAnimRef)
        	ELog.Msg(mtError, "Can't find light animation: %s",anm);
    }

   	m_FalloffTex	= ini.r_string	(sect_name, "fallof_texture");

	if (m_Flags.is(ELight::flPointFuzzy))
    {
        m_FuzzyData	= xr_new<SFuzzyData>();
        m_FuzzyData->LoadLTX(ini, sect_name);
    }

	UpdateTransform	();
	return true;
}

void CLight::SaveLTX(CInifile& ini, LPCSTR sect_name)
{
	CCustomObject::SaveLTX(ini, sect_name);

	ini.w_u16		(sect_name, "version", LIGHT_VERSION);

    ini.w_u32		(sect_name, "type", m_Type);
    ini.w_fcolor	(sect_name, "color", m_Color);
    ini.w_float	    (sect_name, "brightness", m_Brightness);
    ini.w_float	    (sect_name, "range", m_Range);
    ini.w_float	    (sect_name, "attenuation0", m_Attenuation0);
    ini.w_float	    (sect_name, "attenuation1",  m_Attenuation1);
    ini.w_float	    (sect_name, "attenuation2",  m_Attenuation2);
    ini.w_float	    (sect_name, "cone", m_Cone);
    ini.w_float	    (sect_name, "virtual_size", m_VirtualSize);

    ini.w_bool		(sect_name, "use_in_d3d", m_UseInD3D);
    ini.w_u32		(sect_name, "light_flags", m_Flags.get());
    ini.w_u32		(sect_name, "light_control", m_LControl);

	ini.w_string	(sect_name,"anim_ref_name", (m_pAnimRef)?m_pAnimRef->cName.c_str():"");

   	ini.w_string	(sect_name, "fallof_texture", m_FalloffTex.c_str());

	if (m_Flags.is(ELight::flPointFuzzy))
    {
        VERIFY		(m_FuzzyData);
        m_FuzzyData->SaveLTX(ini, sect_name);
    }
}

bool CLight::LoadStream(IReader& F)
{
	u16 version = 0;

    string1024 buf;
    R_ASSERT(F.r_chunk(LIGHT_CHUNK_VERSION,&version));
    if((version!=0x0010)&&(version!=LIGHT_VERSION)){
        ELog.DlgMsg( mtError, "CLight: Unsupported version.");
        return false;
    }

	CCustomObject::LoadStream(F);

    if (F.find_chunk(LIGHT_CHUNK_PARAMS))
    {
        m_Type			= (ELight::EType)F.r_u32();
        F.r_fcolor		(m_Color);  	        
        m_Brightness   	= F.r_float();			
        m_Range			= F.r_float();			
        m_Attenuation0	= F.r_float();			
        m_Attenuation1	= F.r_float();			
        m_Attenuation2	= F.r_float();			
        m_Cone			= F.r_float();
        m_VirtualSize	= F.r_float();
    }else{
	    R_ASSERT(F.find_chunk(LIGHT_CHUNK_D3D_PARAMS));
        Flight			d3d;
	    F.r				(&d3d,sizeof(d3d));
        m_Type			= (ELight::EType)d3d.type;   	        
        m_Color.set		(d3d.diffuse); 	        
        PPosition		= d3d.position;
        m_Range			= d3d.range;			
        m_Attenuation0	= d3d.attenuation0;		
        m_Attenuation1	= d3d.attenuation1;		
        m_Attenuation2	= d3d.attenuation2;		
        m_Cone			= d3d.phi;				
	    R_ASSERT(F.r_chunk(LIGHT_CHUNK_BRIGHTNESS,&m_Brightness));
    }
    
    R_ASSERT			(F.r_chunk(LIGHT_CHUNK_USE_IN_D3D,&m_UseInD3D));

    if (F.find_chunk(LIGHT_CHUNK_FLAG))
    	F.r(&m_Flags.flags,sizeof(m_Flags));

    if (F.find_chunk(LIGHT_CHUNK_LCONTROL))
    	F.r(&m_LControl,sizeof(m_LControl));

	if (D3DLIGHT_DIRECTIONAL==m_Type)
    {
    	ESceneLightTool* lt = dynamic_cast<ESceneLightTool*>(ParentTool); VERIFY(lt);
        lt->m_SunShadowDir.set(FRotation.x,FRotation.y);
        ELog.DlgMsg( mtError, "CLight: Can't load sun.");
    	return false;
    }

    if (F.find_chunk(LIGHT_CHUNK_ANIMREF))
    {
    	F.r_stringZ(buf,sizeof(buf));
        m_pAnimRef	= LALib.FindItem(buf);
        if (!m_pAnimRef) ELog.Msg(mtError, "Can't find light animation: %s",buf);
    }

    if (F.find_chunk(LIGHT_CHUNK_FALLOFF_TEXTURE))
    {
    	F.r_stringZ(m_FalloffTex);
    }

    if (F.find_chunk(LIGHT_CHUNK_FUZZY_DATA))
    {
        m_FuzzyData	= xr_new<SFuzzyData>();
        m_FuzzyData->LoadStream(F);
		m_Flags.set(ELight::flPointFuzzy,TRUE);
    }else{
		m_Flags.set(ELight::flPointFuzzy,FALSE);
    }

	UpdateTransform	();

    return true;
}
//----------------------------------------------------

void CLight::SaveStream(IWriter& F)
{
	CCustomObject::SaveStream(F);

	F.open_chunk	(LIGHT_CHUNK_VERSION);
	F.w_u16			(LIGHT_VERSION);
	F.close_chunk	();

	F.open_chunk	(LIGHT_CHUNK_PARAMS);
    F.w_u32		    (m_Type);     
    F.w_fcolor	    (m_Color);  
    F.w_float	    (m_Brightness);
    F.w_float	    (m_Range);	
    F.w_float	    (m_Attenuation0);
    F.w_float	    (m_Attenuation1);
    F.w_float	    (m_Attenuation2);
    F.w_float	    (m_Cone);
    F.w_float	    (m_VirtualSize);
	F.close_chunk	();

    F.w_chunk		(LIGHT_CHUNK_USE_IN_D3D,&m_UseInD3D,sizeof(m_UseInD3D));
    F.w_chunk		(LIGHT_CHUNK_FLAG,&m_Flags,sizeof(m_Flags));
    F.w_chunk		(LIGHT_CHUNK_LCONTROL,&m_LControl,sizeof(m_LControl));

    if (m_pAnimRef){
		F.open_chunk(LIGHT_CHUNK_ANIMREF);
		F.w_stringZ	(m_pAnimRef->cName);
		F.close_chunk();
    }

    if (m_FalloffTex.size()){
	    F.open_chunk(LIGHT_CHUNK_FALLOFF_TEXTURE);
    	F.w_stringZ	(m_FalloffTex);
	    F.close_chunk();
    }

	if (m_Flags.is(ELight::flPointFuzzy)){
        VERIFY(m_FuzzyData);
        F.open_chunk(LIGHT_CHUNK_FUZZY_DATA);
        m_FuzzyData->SaveStream(F);
        F.close_chunk();
    }
}
//----------------------------------------------------


