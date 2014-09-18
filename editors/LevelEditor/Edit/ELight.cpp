//----------------------------------------------------
// file: ELight.cpp
//----------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "ELight.h"
#include "../ECore/Editor/ui_main.h"
#include "../ECORE/Editor/D3DUtils.h"
#include "scene.h"
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
    LIGHT_CHUNK_SPOT_TEXTURE	= 0xB439,
    LIGHT_CHUNK_FUZZY_DATA		= 0xB440,
    LIGHT_CHUNK_LCONTROL		= 0xB441,
    LIGHT_CHUNK_PARAMS     		= 0xB442,
};
//----------------------------------------------------

#define VIS_RADIUS 		0.25f
#define SEL_COLOR 		0x00FFFFFF
#define NORM_COLOR 		0x00FFFF00
#define NORM_DYN_COLOR 	0x0000FF00
#define LOCK_COLOR 		0x00FF0000

CLight::CLight(LPVOID data, LPCSTR name):CCustomObject(data,name)
{
	Construct(data);
}

void CLight::Construct(LPVOID data)
{
	ClassID 		= OBJCLASS_LIGHT;

    m_UseInD3D		= TRUE;

    m_FuzzyData		= 0;
    
    m_Type 			= ELight::ltPoint;
	m_Color.set		(1.f,1.f,1.f,0);
    m_Brightness 	= 1.f;
	m_Attenuation0 	= 1.f;
	m_Attenuation1 	= 0.f;
	m_Attenuation2 	= 0.f;
	m_Range 		= 8.f;
    m_Cone			= PI_DIV_8;

    m_VirtualSize	= 0.f;

    m_pAnimRef		= 0;
    m_LControl		= 0;

    m_Flags.assign	(ELight::flAffectStatic);
}

CLight::~CLight()
{
	xr_delete		(m_FuzzyData);
}

void CLight::OnUpdateTransform()
{
    FScale.set		(1.f,1.f,1.f);
	inherited::OnUpdateTransform();
}

void CLight::CopyFrom(CLight* src)
{
	THROW2("CLight:: Go to AlexMX");
}

void CLight::AffectD3D(BOOL flag){
	m_UseInD3D = flag;
    UI->UpdateScene();
}
//----------------------------------------------------

bool CLight::GetBox( Fbox& box ) const
{
	box.set		(PPosition, PPosition);
	box.min.sub	(m_Range);
	box.max.add	(m_Range);
	return true;
}

void CLight::Render(int priority, bool strictB2F)
{
	inherited::Render(priority,strictB2F);
    if ((1==priority)&&(false==strictB2F)){
        EDevice.SetShader		(EDevice.m_WireShader);
        RCache.set_xform_world	(Fidentity);
    	u32 clr = Selected()?SEL_COLOR:(m_Flags.is(ELight::flAffectDynamic)?NORM_DYN_COLOR:NORM_COLOR);
    	switch (m_Type){
        case ELight::ltPoint:
            if (Selected())
            	DU_impl.DrawLineSphere( PPosition, m_Range, clr, true );

            DU_impl.DrawPointLight(PPosition,VIS_RADIUS, clr);
            if (m_Flags.is(ELight::flPointFuzzy)){
            	VERIFY(m_FuzzyData);
			    for (FvectorIt it=m_FuzzyData->m_Positions.begin(); it!=m_FuzzyData->m_Positions.end(); it++){
                	Fvector tmp; _Transform().transform_tiny(tmp,*it);
		            DU_impl.DrawPointLight(tmp,VIS_RADIUS/6, clr);
	            }
			}
        break;
        case ELight::ltSpot:{
//			Fvector dir;
//			dir.setHP		(PRotation.y,PRotation.x);
//			DU.DrawCone		(Fidentity, PPosition, dir, Selected()?m_Range:VIS_RADIUS, radius2, clr, true, false);
        	if (Selected())
            	DU_impl.DrawSpotLight( PPosition, FTransformR.k, m_Range, m_Cone, clr );
            else
            	DU_impl.DrawSpotLight( PPosition, FTransformR.k, VIS_RADIUS, m_Cone, clr );
        }break;
        default: THROW;
        }
    	ESceneLightTool* lt = dynamic_cast<ESceneLightTool*>(ParentTool);
        VERIFY				(lt);
        
        if (lt->m_Flags.is(ESceneLightTool::flShowControlName))
        {
            Fvector 		D;
            D.sub			(EDevice.vCameraPosition,PPosition);
            float dist 		= D.normalize_magn();
        	if (!Scene->RayPickObject(dist,PPosition,D,OBJCLASS_SCENEOBJECT,0,0))
	        	DU_impl.OutText (PPosition,AnsiString().sprintf(" %s",GetLControlName()).c_str(),0xffffffff,0xff000000);
        }
    }else if ((1==priority)&&(true==strictB2F))
    {
        EDevice.SetShader		(EDevice.m_SelectionShader);
        RCache.set_xform_world	(Fidentity);
    	switch (m_Type)
        {
        case ELight::ltPoint:
            if (m_Flags.is(ELight::flPointFuzzy))
            {
		    	u32 clr = Selected()?SEL_COLOR:(m_Flags.is(ELight::flAffectDynamic)?NORM_DYN_COLOR:NORM_COLOR);
                clr 	= subst_alpha(clr,0x40);
            	const Fvector zero={0.f,0.f,0.f};
                VERIFY(m_FuzzyData);
                switch (m_FuzzyData->m_ShapeType)
                {
                case CLight::SFuzzyData::fstSphere:
                	DU_impl.DrawSphere	(_Transform(),zero,m_FuzzyData->m_SphereRadius,clr,clr,true,true);
                break;
                case CLight::SFuzzyData::fstBox:
                	DU_impl.DrawAABB		(_Transform(),zero,m_FuzzyData->m_BoxDimension,clr,clr,true,true);
                break;
                }
			}
        break;
        case ELight::ltSpot:		break;
        default: THROW;
        }
	}
}

bool CLight::FrustumPick(const CFrustum& frustum)
{
//    return (frustum.testSphere(m_Position,m_Range))?true:false;
    return (frustum.testSphere_dirty(PPosition,VIS_RADIUS))?true:false;
}

bool CLight::RayPick(float& distance, const Fvector& start, const Fvector& direction, SRayPickInfo* pinf)
{
	Fvector ray2;
	ray2.sub( PPosition, start );

    float d = ray2.dotproduct(direction);
    if( d > 0  ){
        float d2 = ray2.magnitude();
        if( ((d2*d2-d*d) < (VIS_RADIUS*VIS_RADIUS)) && (d>VIS_RADIUS) ){
        	if (d<distance){
	            distance = d;
    	        return true;
            }
        }
    }
	return false;
}
//----------------------------------------------------

void CLight::OnFrame	(){
	inherited::OnFrame	();
}
//----------------------------------------------------

void CLight::Update()
{
	UpdateTransform();
}
//----------------------------------------------------

LPCSTR CLight::GetLControlName()
{
    ESceneLightTool* lt		= dynamic_cast<ESceneLightTool*>(ParentTool); VERIFY(lt);
    xr_rtoken* lc			= lt->FindLightControl(m_LControl);
	return lc?*lc->name:0;
}
//----------------------------------------------------

void CLight::OnNeedUpdate(PropValue* value)
{
	Update();
}
//----------------------------------------------------

bool CLight::GetSummaryInfo(SSceneSummary* inf)
{
	inherited::GetSummaryInfo	(inf);
    switch (m_Type){
    case ELight::ltPoint:		inf->light_point_cnt++; break;
    case ELight::ltSpot:		inf->light_spot_cnt++; 	break;
    }

    switch (m_Type){
    case ELight::ltPoint:
    case ELight::ltSpot:
        if (m_Flags.is(ELight::flAffectStatic))		inf->light_static_cnt++;
        if (m_Flags.is(ELight::flAffectDynamic))	inf->light_dynamic_cnt++;
        if (m_Flags.is(ELight::flProcedural))		inf->light_procedural_cnt++;
        if (m_Flags.is(ELight::flBreaking))			inf->light_breakable_cnt++;
        break;
    }
	return true;
}
//----------------------------------------------------

