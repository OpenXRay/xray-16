#include "stdafx.h"
#pragma hdrstop

#include "postprocessanimator.h"
#ifndef _PP_EDITOR_
#include "ActorEffector.h"	
#endif

// postprocess value LOAD method implementation
void CPostProcessValue::load (IReader &pReader)
{
    m_Value.Load_2 (pReader);
}

void CPostProcessValue::save (IWriter &pWriter)
{
    m_Value.Save (pWriter);
}
 
// postprocess color LOAD method implementation
void CPostProcessColor::load (IReader &pReader)
{
    m_fBase = pReader.r_float	();
    m_Red.Load_2				(pReader);
    m_Green.Load_2				(pReader);
    m_Blue.Load_2				(pReader);
}

void CPostProcessColor::save (IWriter &pWriter)
{
    pWriter.w_float				(m_fBase);
    m_Red.Save					(pWriter);
    m_Green.Save				(pWriter);
    m_Blue.Save					(pWriter);
}

//main PostProcessAnimator class

CPostprocessAnimator::CPostprocessAnimator()
{
    Create				();
}

CPostprocessAnimator::CPostprocessAnimator(int id, bool cyclic)
#ifndef _PP_EDITOR_
:CEffectorPP((EEffectorPPType)id, 100000, true),
m_bCyclic(cyclic)
#endif
{
    Create				();
}

CPostprocessAnimator::~CPostprocessAnimator           ()
{
    Clear ();
}

#ifndef _PP_EDITOR_
BOOL CPostprocessAnimator::Valid()
{
	if(m_bCyclic)	return TRUE;

	return CEffectorPP::Valid	();
}
#endif

void        CPostprocessAnimator::Clear                           ()
{
    for (int a = 0; a < POSTPROCESS_PARAMS_COUNT; a++)
        xr_delete (m_Params[a]);
}

void        CPostprocessAnimator::Load                            (LPCSTR name)
{
    m_Name = name;
#ifndef _PP_EDITOR_
    string_path full_path;
    if (!FS.exist (full_path, "$level$", name))
       if (!FS.exist (full_path, "$game_anims$", name))
          Debug.fatal (DEBUG_INFO,"Can't find motion file '%s'.", name);
#else /*_PP_EDITOR_*/
    string_path full_path;
    xr_strcpy (full_path, name);
#endif /*_PP_EDITOR_*/

    LPCSTR  ext = strext(full_path);
    if (ext)
       {
       if (!xr_strcmp (ext,POSTPROCESS_FILE_EXTENSION))
          {
          IReader* F = FS.r_open (full_path);
          u32 dwVersion = F->r_u32();
//.       VERIFY (dwVersion == POSTPROCESS_FILE_VERSION);
          //load base color
          VERIFY (m_Params[0]);
          m_Params[0]->load (*F);
          //load add color
          VERIFY (m_Params[1]);
          m_Params[1]->load (*F);
          //load gray color
          VERIFY (m_Params[2]);
          m_Params[2]->load (*F);
          //load gray value
          VERIFY (m_Params[3]);
          m_Params[3]->load (*F);
          //load blur value
          VERIFY (m_Params[4]);
          m_Params[4]->load (*F);
          //load duality horizontal
          VERIFY (m_Params[5]);
          m_Params[5]->load (*F);
          //load duality vertical
          VERIFY (m_Params[6]);
          m_Params[6]->load (*F);
          //load noise intensity
          VERIFY (m_Params[7]);
          m_Params[7]->load (*F);
          //load noise granularity
          VERIFY (m_Params[8]);
          m_Params[8]->load (*F);
          //load noise fps
          VERIFY (m_Params[9]);
          m_Params[9]->load (*F);
		  if(dwVersion>=0x0002)
		  {
			  VERIFY (m_Params[10]);
			  m_Params[10]->load (*F);
			  F->r_stringZ(m_EffectorParams.cm_tex1);
		  }
          //close reader
          FS.r_close (F);
          }
        else
           FATAL	("ERROR: Can't support files with many animations set. Incorrect file.");
        }

	f_length					= GetLength	();
#ifndef _PP_EDITOR_
	if(!m_bCyclic)	fLifeTime	= f_length;
#endif

}

void        CPostprocessAnimator::Stop       (float sp)
{
	if(m_bStop)			return;
	m_bStop				= true;
	VERIFY				(_valid(sp));
	m_factor_speed		= sp;
}

float       CPostprocessAnimator::GetLength                       ()
{
    float v = 0.0f;
    for (int a = 0; a < POSTPROCESS_PARAMS_COUNT; a++)
        {
        float t = m_Params[a]->get_length();
        v		= _max(t,v);
        }
    return v;
}

void        CPostprocessAnimator::Update                          (float tm)
{
    for (int a = 0; a < POSTPROCESS_PARAMS_COUNT; a++)
        m_Params[a]->update (tm);
}
void CPostprocessAnimator::SetDesiredFactor	(float f, float sp)			
{
	m_dest_factor=f;
	m_factor_speed=sp;
	VERIFY				(_valid(m_factor));
	VERIFY				(_valid(m_dest_factor));
};

void CPostprocessAnimator::SetCurrentFactor	(float f)					
{
	m_factor=f;
	m_dest_factor=f;
	VERIFY				(_valid(m_factor));
	VERIFY				(_valid(m_dest_factor));
};

#ifndef _PP_EDITOR_
BOOL CPostprocessAnimator::Process(SPPInfo &PPInfo)
{
	if(m_bCyclic)
		fLifeTime				= 100000;

	CEffectorPP::Process		(PPInfo);
	

	if(m_start_time<0.0f)m_start_time=Device.fTimeGlobal;
	if(m_bCyclic &&((Device.fTimeGlobal-m_start_time)>f_length)) m_start_time+=f_length;

	Update					(Device.fTimeGlobal-m_start_time);

	VERIFY				(_valid(m_factor));
	VERIFY				(_valid(m_factor_speed));
	VERIFY				(_valid(m_dest_factor));
	if(m_bStop)
		m_factor			-=	Device.fTimeDelta*m_factor_speed;
	else
		m_factor			+= m_factor_speed*Device.fTimeDelta*(m_dest_factor-m_factor);

	clamp					(m_factor, 0.0001f, 1.0f);

	VERIFY				(_valid(m_factor));
	VERIFY				(_valid(m_factor_speed));

	m_EffectorParams.color_base		+= pp_identity.color_base;
	m_EffectorParams.color_gray		+= pp_identity.color_gray;
	m_EffectorParams.color_add		+= pp_identity.color_add;

	if(0==m_Params[pp_noise_i]->get_keys_count()){
		m_EffectorParams.noise.intensity = pp_identity.noise.intensity;
	}
	
	if(0==m_Params[pp_noise_g]->get_keys_count()){
		m_EffectorParams.noise.grain = pp_identity.noise.grain;
	}

	if(0==m_Params[pp_noise_f]->get_keys_count()){
		m_EffectorParams.noise.fps = pp_identity.noise.fps;
	}else
		m_EffectorParams.noise.fps		*= 100.0f;

	PPInfo.lerp				(pp_identity, m_EffectorParams, m_factor);

	if(PPInfo.noise.grain<=0.0f){
		R_ASSERT3(0,"noise.grain cant be zero! see postprocess",*m_Name);
	}

	if(fsimilar(m_factor,0.0001f,EPS_S))
		return FALSE;

	return TRUE;
}
#else
BOOL CPostprocessAnimator::Process(float dt, SPPInfo &PPInfo)
{

	Update					(dt);

	//if(m_bStop)
//		m_factor			-=	dt*m_stop_speed;

	clamp					(m_factor, 0.001f, 1.0f);

    PPInfo = m_EffectorParams;
//	PPInfo.lerp				(pp_identity, m_EffectorParams, m_factor);

//	if(fsimilar(m_factor,0.001f,EPS_S))
//		return FALSE;

	return TRUE;
}
#endif /*_PP_EDITOR_*/

void        CPostprocessAnimator::Create                          ()
{
	m_factor			= 1.0f;
	m_dest_factor		= 1.0f;
	m_bStop				= false;
	m_start_time		= -1.0f;
	m_factor_speed		= 1.0f;
	f_length			= 0.0f;

    m_Params[0] = xr_new<CPostProcessColor> (&m_EffectorParams.color_base);			//base color
    VERIFY (m_Params[0]);
    m_Params[1] = xr_new<CPostProcessColor> (&m_EffectorParams.color_add);          //add color
    VERIFY (m_Params[1]);
    m_Params[2] = xr_new<CPostProcessColor> (&m_EffectorParams.color_gray);         //gray color
    VERIFY (m_Params[2]);
    m_Params[3] = xr_new<CPostProcessValue> (&m_EffectorParams.gray);              //gray value
    VERIFY (m_Params[3]);
    m_Params[4] = xr_new<CPostProcessValue> (&m_EffectorParams.blur);              //blur value
    VERIFY (m_Params[4]);
    m_Params[5] = xr_new<CPostProcessValue> (&m_EffectorParams.duality.h);          //duality horizontal
    VERIFY (m_Params[5]);
    m_Params[6] = xr_new<CPostProcessValue> (&m_EffectorParams.duality.v);          //duality vertical
    VERIFY (m_Params[6]);
    m_Params[7] = xr_new<CPostProcessValue> (&m_EffectorParams.noise.intensity);    //noise intensity
    VERIFY (m_Params[7]);
    m_Params[8] = xr_new<CPostProcessValue> (&m_EffectorParams.noise.grain);        //noise granularity
    VERIFY (m_Params[8]);
    m_Params[9] = xr_new<CPostProcessValue> (&m_EffectorParams.noise.fps);          //noise fps
    VERIFY (m_Params[9]);
    m_Params[10] = xr_new<CPostProcessValue> (&m_EffectorParams.cm_influence);
    VERIFY (m_Params[10]);
}

#ifdef _PP_EDITOR_
CPostProcessParam*  CPostprocessAnimator::GetParam                (pp_params param)
{
    VERIFY (param >= pp_base_color && param < pp_last);
    return m_Params[param];
}
void        CPostprocessAnimator::Save                            (LPCSTR name)
{
    IWriter *W = FS.w_open (name);
    VERIFY (W);
    W->w_u32(POSTPROCESS_FILE_VERSION);
    m_Params[0]->save (*W);
    m_Params[1]->save (*W);
    m_Params[2]->save (*W);
    m_Params[3]->save (*W);
    m_Params[4]->save (*W);
    m_Params[5]->save (*W);
    m_Params[6]->save (*W);
    m_Params[7]->save (*W);
    m_Params[8]->save (*W);
    m_Params[9]->save (*W);
    m_Params[10]->save (*W);
	W->w_stringZ		(m_EffectorParams.cm_tex1);
    FS.w_close			(W);
}

SPPInfo::SPPInfo				()
{
	blur = gray = duality.h = duality.v = 0;
	noise.intensity=0; noise.grain = 1; noise.fps = 10;
	color_base.set	(.5f,	.5f,	.5f);
	color_gray.set	(.333f, .333f,	.333f);
	color_add.set	(0.f,	0.f,	0.f);
	cm_influence	= 0.0f;
	cm_interpolate	= 1.0f;
}

//-----------------------------------------------------------------------
void        CPostprocessAnimator::ResetParam                      (pp_params param)
{
    xr_delete (m_Params[param]);
    switch (param)
           {
           case pp_base_color:
                m_Params[0] = xr_new<CPostProcessColor> (&m_EffectorParams.color_base);   //base color
                break;
           case pp_add_color:
                m_Params[1] = xr_new<CPostProcessColor> (&m_EffectorParams.color_add);          //add color
                break;
           case pp_gray_color:
                m_Params[2] = xr_new<CPostProcessColor> (&m_EffectorParams.color_gray);         //gray color
                break;
           case pp_gray_value:
                m_Params[3] = xr_new<CPostProcessValue> (&m_EffectorParams.gray);              //gray value
                break;
           case pp_blur:
                m_Params[4] = xr_new<CPostProcessValue> (&m_EffectorParams.blur);              //blur value
                break;
           case pp_dual_h:
                m_Params[5] = xr_new<CPostProcessValue>  (&m_EffectorParams.duality.h);       //duality horizontal
                break;
           case pp_dual_v:
                m_Params[6] = xr_new<CPostProcessValue>  (&m_EffectorParams.duality.v);       //duality vertical
                break;
           case pp_noise_i:
                m_Params[7] = xr_new<CPostProcessValue>  (&m_EffectorParams.noise.intensity);         //noise intensity
                break;
           case pp_noise_g:
                m_Params[8] = xr_new<CPostProcessValue>  (&m_EffectorParams.noise.grain);         //noise granularity
                break;
           case pp_noise_f:
                m_Params[9] = xr_new<CPostProcessValue>  (&m_EffectorParams.noise.fps);         //noise fps
                break;
           case pp_cm_influence:
                m_Params[10] = xr_new<CPostProcessValue>  (&m_EffectorParams.cm_influence);
                break;
           }
    VERIFY (m_Params[param]);
}

void CPostProcessColor::clear_all_keys()
{
	m_Red.ClearAndFree();
	m_Green.ClearAndFree();
	m_Blue.ClearAndFree();
}

void CPostProcessColor::delete_value(float time)
{
   m_Red.DeleteKey (time);
   m_Green.DeleteKey(time);
   m_Blue.DeleteKey(time);
}

void CPostProcessColor::add_value(float time, float value, int index)
{
    KeyIt i;
    if (0 == index)
       {
       m_Red.InsertKey (time, value);
       i = m_Red.FindKey (time, 0.01f);
       }
    else if (1 == index)
            {
            m_Green.InsertKey (time, value);
            i = m_Green.FindKey (time, 0.01f);
            }
         else
            {
            m_Blue.InsertKey (time, value);
            i = m_Blue.FindKey (time, 0.01f);
            }
    (*i)->tension 		= 0;
    (*i)->continuity 	= 0;
    (*i)->bias 			= 0;
}

void CPostProcessColor::update_value(float time, float value, int index)
{
    KeyIt i;
    if (0 == index) i = m_Red.FindKey (time, 0.01f);
    else if (1 == index) i = m_Green.FindKey (time, 0.01f);
         else i = m_Blue.FindKey (time, 0.01f);
         
    (*i)->value 		= value;
    (*i)->tension 		= 0;
    (*i)->continuity 	= 0;
    (*i)->bias 			= 0;
}

void CPostProcessColor::get_value(float time, float &value, int index)
{
    KeyIt i;
    if (0 == index) i = m_Red.FindKey (time, 0.01f);
    else if (1 == index) i = m_Green.FindKey (time, 0.01f);
         else i = m_Blue.FindKey (time, 0.01f);
    value = (*i)->value;
}

void CPostProcessValue::delete_value(float time)
{
    m_Value.DeleteKey (time);
}

void CPostProcessValue::clear_all_keys()
{
	m_Value.ClearAndFree();
}

void CPostProcessValue::add_value(float time, float value, int index)
{
    m_Value.InsertKey (time, value);
    KeyIt i = m_Value.FindKey (time, 0.01f);

    (*i)->tension 		= 0;
    (*i)->continuity 	= 0;
    (*i)->bias 			= 0;
}

void CPostProcessValue::update_value(float time, float value, int index)
{
    KeyIt i = m_Value.FindKey (time, 0.01f);
    (*i)->value 		= value;
    (*i)->tension 		= 0;
    (*i)->continuity 	= 0;
    (*i)->bias 			= 0;
}

void CPostProcessValue::get_value(float time, float &value, int index)
{
    KeyIt i = m_Value.FindKey (time, 0.01f);
    value = (*i)->value;
}
#endif /*_PP_EDITOR_*/


#ifndef _PP_EDITOR_
BOOL CPostprocessAnimatorLerp::Process(SPPInfo &PPInfo)
{
	if(!m_bStop)
		m_factor = m_get_factor_func			();
	return CPostprocessAnimator::Process		(PPInfo);
}


BOOL CPostprocessAnimatorLerpConst::Process(SPPInfo &PPInfo)
{
	if(!m_bStop)
		m_factor = m_power;
	return CPostprocessAnimator::Process		(PPInfo);
}

CPostprocessAnimatorControlled::CPostprocessAnimatorControlled(CEffectorController* c)
:m_controller(c)
{
	m_controller->SetPP(this);
	SetFactorFunc(fastdelegate::FastDelegate0<float>(m_controller, &CEffectorController::GetFactor));
}

CPostprocessAnimatorControlled::~CPostprocessAnimatorControlled()
{
	m_controller->SetPP(NULL);
}

BOOL CPostprocessAnimatorControlled::Valid()
{
	return m_controller->Valid					();
}

#endif