#ifndef __ppanimator_included__
#define __ppanimator_included__
#pragma once

#ifndef _PP_EDITOR_
    #include "../xrEngine/envelope.h"
    #include "../xrEngine/EffectorPP.h"
	#include "../xrEngine/cameramanager.h"

	class CEffectorController;
#else
    #include "envelope.h"
    #include "EffectorPP.h"
    #include "CameraManager.h"
#endif /*_PP_EDITOR_*/

#define POSTPROCESS_PARAMS_COUNT    11
//.#define POSTPROCESS_FILE_VERSION    0x0001
#define POSTPROCESS_FILE_VERSION    0x0002

#define POSTPROCESS_FILE_EXTENSION  ".ppe"


typedef enum _pp_params
{
    pp_unknown              =  -1,
    pp_base_color           =   0,
    pp_add_color            =   1,
    pp_gray_color           =   2,
    pp_gray_value           =   3,
    pp_blur                 =   4,
    pp_dual_h               =   5,
    pp_dual_v               =   6,
    pp_noise_i              =   7,
    pp_noise_g              =   8,
    pp_noise_f              =   9,
    pp_cm_influence         =   10,
    pp_last					=	11,
    pp_force_dword          =   0x7fffffff
} pp_params;


class CPostProcessParam
{
protected:
public:
    virtual void    update                          (float dt) = 0;
    virtual void    load                            (IReader &pReader) = 0;
    virtual void    save                            (IWriter &pWriter) = 0;
    virtual float   get_length                      () = 0;
    virtual size_t  get_keys_count                  () = 0;
	virtual ~CPostProcessParam() {}
#ifdef _PP_EDITOR_
    virtual void    add_value                       (float time, float value,  int index = 0) = 0;
    virtual void    delete_value                    (float time) = 0;
    virtual void    update_value                    (float time, float value,  int index = 0) = 0;
    virtual void    get_value                       (float time, float &value, int index = 0) = 0;
    virtual float   get_key_time                    (size_t index) = 0;
    virtual void   clear_all_keys                   () = 0;
#endif /*_PP_EDITOR_*/
};

class CPostProcessValue : public CPostProcessParam
{
protected:
    CEnvelope       m_Value;
    float          *m_pfParam;
public:
                    CPostProcessValue               (float *pfparam) 		{m_pfParam = pfparam;}
    virtual void    update                          (float dt)
                    {
                    *m_pfParam = m_Value.Evaluate (dt);
                    }
    virtual void    load                            (IReader &pReader);
    virtual void    save                            (IWriter &pWriter);
    virtual float   get_length                      ()                    	{float mn, mx; return m_Value.GetLength (&mn, &mx);}
    virtual size_t  get_keys_count                  () 						{return m_Value.keys.size ();}
#ifdef _PP_EDITOR_
    virtual void    add_value                       (float time, float value, int index = 0);
    virtual void    delete_value                    (float time);
    virtual void    update_value                    (float time, float value, int index = 0);
    virtual void    get_value                       (float time, float &valueb, int index = 0);
    virtual float   get_key_time                    (size_t index)  {VERIFY (index < get_keys_count ()); return m_Value.keys[index]->time;}
    virtual void   clear_all_keys                  ();
#endif /*_PP_EDITOR_*/
};


class CPostProcessColor : public CPostProcessParam
{
protected:
    float           m_fBase;
    CEnvelope       m_Red;
    CEnvelope       m_Green;
    CEnvelope       m_Blue;
	SPPInfo::SColor *m_pColor;
public:
                    CPostProcessColor               (SPPInfo::SColor *pcolor) { m_pColor = pcolor; }
    virtual void    update                          (float dt)
                    {
                    m_pColor->r = m_Red.Evaluate (dt);
                    m_pColor->g = m_Green.Evaluate (dt);
                    m_pColor->b = m_Blue.Evaluate (dt);
                    }
    virtual void    load                            (IReader &pReader);
    virtual void    save                            (IWriter &pWriter);
    virtual float   get_length                      ()
                    {
                    float mn, mx,
                    r = m_Red.GetLength (&mn, &mx),
                    g = m_Green.GetLength (&mn, &mx),
                    b = m_Blue.GetLength (&mn, &mx);
                    mn = (r > g ? r : g);
                    return mn > b ? mn : b;
                    }
    virtual size_t  get_keys_count                  ()
                    {
                    return m_Red.keys.size ();
                    }
#ifdef _PP_EDITOR_
    virtual void    add_value                       (float time, float value, int index = 0);
    virtual void    delete_value                    (float time);
    virtual void    update_value                    (float time, float value, int index = 0);
    virtual void    get_value                       (float time, float &value, int index = 0);
    virtual float   get_key_time                    (size_t index)
                    {
                    VERIFY (index < get_keys_count ());
                    return m_Red.keys[index]->time;
                    }
    virtual void   clear_all_keys                  ();
#endif /*_PP_EDITOR_*/
};


#ifndef _PP_EDITOR_
class CPostprocessAnimator :public CEffectorPP
#else
class CPostprocessAnimator
#endif
{
protected:
    SPPInfo											m_EffectorParams;
    CPostProcessParam                               *m_Params[POSTPROCESS_PARAMS_COUNT];
    shared_str										m_Name;
	float											m_factor;
	float											m_dest_factor;
	bool											m_bStop;
	float											m_factor_speed;
	bool											m_bCyclic;
	float											m_start_time;
	float                                           f_length;

	void		Update								(float tm);
public:
                    CPostprocessAnimator            (int id, bool cyclic);
                    CPostprocessAnimator            ();
        virtual    ~CPostprocessAnimator            ();
        void        Clear                           ();
        void        Load                            (LPCSTR name);
    IC  LPCSTR      Name                            (){return *m_Name;}
  virtual void      Stop                            (float speed);
		void		SetDesiredFactor				(float f, float sp);
		void		SetCurrentFactor				(float f);
		void		SetCyclic						(bool b)					{m_bCyclic=b;}
        float       GetLength                       ();
        SPPInfo&	PPinfo							() {return m_EffectorParams;}
#ifndef _PP_EDITOR_
virtual	BOOL		Valid							();
virtual	BOOL		Process							(SPPInfo &PPInfo);
#else
virtual	BOOL		Process							(float dt, SPPInfo &PPInfo);
#endif /*_PP_EDITOR_*/
        void        Create                          ();
#ifdef _PP_EDITOR_
        CPostProcessParam*  GetParam                (pp_params param);
        void        ResetParam                      (pp_params param);
        void        Save                            (LPCSTR name);
#endif /*_PP_EDITOR_*/
};

#ifndef _PP_EDITOR_
class CPostprocessAnimatorLerp :public CPostprocessAnimator
{
protected:
		fastdelegate::FastDelegate0<float>	m_get_factor_func;
public:
	void			SetFactorFunc				(fastdelegate::FastDelegate0<float> f)	{m_get_factor_func=f;}
virtual	BOOL		Process						(SPPInfo &PPInfo);
};

class CPostprocessAnimatorLerpConst :public CPostprocessAnimator
{
protected:
		float		m_power;
public:
					CPostprocessAnimatorLerpConst	()					{m_power = 1.0f;}
		void		SetPower						(float val)			{m_power=val;}
virtual	BOOL		Process							(SPPInfo &PPInfo);
};

class CPostprocessAnimatorControlled :public CPostprocessAnimatorLerp
{
	CEffectorController*		m_controller;
public:
	virtual				~CPostprocessAnimatorControlled		();
						CPostprocessAnimatorControlled		(CEffectorController* c);
	virtual BOOL		Valid								();
};

#endif

#endif /*__ppanimator_included__*/
