#ifndef EnvironmentH
#define EnvironmentH

// refs
class ENGINE_API	IRender_Visual;
class ENGINE_API	CInifile;
class ENGINE_API 	CEnvironment;

// refs - effects
class ENGINE_API	CEnvironment;
class ENGINE_API	CLensFlare;
class ENGINE_API	CEffect_Rain;
class ENGINE_API	CEffect_Thunderbolt;

class ENGINE_API	CPerlinNoise1D;

struct SThunderboltDesc;
struct SThunderboltCollection;
class CLensFlareDescriptor;

#define DAY_LENGTH		86400.f

#include "../Include/xrRender/FactoryPtr.h"
#include "../Include/xrRender/EnvironmentRender.h"

#ifdef INGAME_EDITOR
#	define	INGAME_EDITOR_VIRTUAL	virtual
#else // #ifdef INGAME_EDITOR
#	define	INGAME_EDITOR_VIRTUAL
#endif // #ifdef INGAME_EDITOR

// t-defs
class ENGINE_API	CEnvModifier
{
public:
	Fvector3			position;
	float				radius;
	float				power;

	float				far_plane;
	Fvector3			fog_color;
	float				fog_density;
	Fvector3			ambient;
	Fvector3			sky_color;		
	Fvector3			hemi_color;
	Flags16				use_flags;

	void				load		(IReader* fs, u32 version);
	float				sum			(CEnvModifier&	_another, Fvector3& view);
};

class ENGINE_API	CEnvAmbient
{
public:
	struct SEffect{
		u32 			life_time;
		ref_sound		sound;		
		shared_str		particles;
		Fvector			offset;
		float			wind_gust_factor;
		float			wind_blast_in_time;
		float			wind_blast_out_time;
		float			wind_blast_strength;
		Fvector			wind_blast_direction;

		INGAME_EDITOR_VIRTUAL	~SEffect				()	{}
	};
	DEFINE_VECTOR(SEffect*,EffectVec,EffectVecIt);
	struct SSndChannel
	{
		shared_str				m_load_section;
		Fvector2				m_sound_dist;
		Ivector4				m_sound_period;

		typedef xr_vector<ref_sound>	sounds_type;

		void					load					(CInifile& config, LPCSTR sect);
		ref_sound&				get_rnd_sound			()	{return sounds()[Random.randI(sounds().size())];}
		u32						get_rnd_sound_time		()	{return (m_sound_period.z < m_sound_period.w) ? Random.randI(m_sound_period.z,m_sound_period.w) : 0;}
		u32						get_rnd_sound_first_time()	{return (m_sound_period.x < m_sound_period.y) ? Random.randI(m_sound_period.x,m_sound_period.y) : 0;}
		float					get_rnd_sound_dist		()	{return (m_sound_dist.x < m_sound_dist.y) ? Random.randF(m_sound_dist.x, m_sound_dist.y) : 0;}
		INGAME_EDITOR_VIRTUAL	~SSndChannel			()	{}
		inline INGAME_EDITOR_VIRTUAL sounds_type& sounds()  {return m_sounds;}

	protected:
		xr_vector<ref_sound>	m_sounds;
	};
	DEFINE_VECTOR(SSndChannel*,SSndChannelVec,SSndChannelVecIt);
protected:
	shared_str				m_load_section;

	EffectVec				m_effects;
	Ivector2				m_effect_period;

	SSndChannelVec			m_sound_channels;
	shared_str              m_ambients_config_filename;

public:
	IC const shared_str&	name				()	{return m_load_section;}
	IC const shared_str&	get_ambients_config_filename ()	{return m_ambients_config_filename;}

	INGAME_EDITOR_VIRTUAL	void	load		(
								CInifile& ambients_config,
								CInifile& sound_channels_config,
								CInifile& effects_config,
								const shared_str& section
							);
	IC SEffect*				get_rnd_effect		()	{return effects().empty()?0:effects()[Random.randI(effects().size())];}
	IC u32					get_rnd_effect_time ()	{return Random.randI(m_effect_period.x, m_effect_period.y);}

	INGAME_EDITOR_VIRTUAL	SEffect*		create_effect			(CInifile& config, LPCSTR id);
	INGAME_EDITOR_VIRTUAL	SSndChannel*	create_sound_channel	(CInifile& config, LPCSTR id);
	INGAME_EDITOR_VIRTUAL					~CEnvAmbient			();
							void			destroy					();
	inline INGAME_EDITOR_VIRTUAL EffectVec&			effects			() { return m_effects; }
	inline INGAME_EDITOR_VIRTUAL SSndChannelVec&	get_snd_channels() { return m_sound_channels; }
};

class ENGINE_API	CEnvDescriptor
{
public:
	float				exec_time;
	float				exec_time_loaded;

	shared_str			sky_texture_name	;
	shared_str			sky_texture_env_name;
	shared_str			clouds_texture_name	;

	BENCH_SEC_SCRAMBLEMEMBER1

	/*
	ref_texture			sky_texture		;
	ref_texture			sky_texture_env	;
	ref_texture			clouds_texture	;
	*/
	FactoryPtr<IEnvDescriptorRender>	m_pDescriptor;

	Fvector4			clouds_color	;
	Fvector3			sky_color		;
	float				sky_rotation	;

	float				far_plane;

	Fvector3			fog_color;
	float				fog_density;
	float				fog_distance;

	float				rain_density;
	Fvector3			rain_color;

	float				bolt_period;
	float				bolt_duration;

    float				wind_velocity;
    float				wind_direction;  
    
	Fvector3			ambient		;
	Fvector4			hemi_color	;	// w = R2 correction
	Fvector3			sun_color	;
	Fvector3			sun_dir		;
	float				m_fSunShaftsIntensity;
	float				m_fWaterIntensity;


//	int					lens_flare_id;
//	int					tb_id;
	shared_str			lens_flare_id;
	shared_str			tb_id;
    
	CEnvAmbient*		env_ambient;


						CEnvDescriptor	(shared_str const& identifier);

	void				load			(CEnvironment& environment, CInifile& config);
	void				copy			(const CEnvDescriptor& src)
	{
		float tm0		= exec_time;
		float tm1		= exec_time_loaded; 
		*this			= src;
		exec_time		= tm0;
		exec_time_loaded= tm1;
	}

	void				on_device_create	();
	void				on_device_destroy	();

	shared_str			m_identifier;
};

class ENGINE_API		CEnvDescriptorMixer: public CEnvDescriptor{
public:
	/*
	STextureList		sky_r_textures;		
	STextureList		sky_r_textures_env;	
	STextureList		clouds_r_textures;	
	*/
	FactoryPtr<IEnvDescriptorMixerRender>	m_pDescriptorMixer;
	float				weight;				

	float				fog_near;		
	float				fog_far;		
public:
						CEnvDescriptorMixer	(shared_str const& identifier);
	INGAME_EDITOR_VIRTUAL void lerp			(CEnvironment* parent, CEnvDescriptor& A, CEnvDescriptor& B, float f, CEnvModifier& M, float m_power);
	void				clear				();
	void				destroy				();
};

class ENGINE_API	CEnvironment
{
	friend class dxEnvironmentRender;
	struct str_pred : public std::binary_function<shared_str, shared_str, bool>	{	
		IC bool operator()(const shared_str& x, const shared_str& y) const
		{	return xr_strcmp(x,y)<0;	}
	};
public:
	DEFINE_VECTOR			(CEnvAmbient*,EnvAmbVec,EnvAmbVecIt);
	DEFINE_VECTOR			(CEnvDescriptor*,EnvVec,EnvIt);
	DEFINE_MAP_PRED			(shared_str,EnvVec,EnvsMap,EnvsMapIt,str_pred);
private:
	// clouds
	FvectorVec				CloudsVerts;
	U16Vec					CloudsIndices;
private:
	float					NormalizeTime	(float tm);
	float					TimeDiff		(float prev, float cur);
	float					TimeWeight		(float val, float min_t, float max_t);
	void					SelectEnvs		(EnvVec* envs, CEnvDescriptor*& e0, CEnvDescriptor*& e1, float tm);
	void					SelectEnv		(EnvVec* envs, CEnvDescriptor*& e, float tm);

	void					calculate_dynamic_sun_dir();
public:
	static bool sort_env_pred	(const CEnvDescriptor* x, const CEnvDescriptor* y)
	{	return x->exec_time < y->exec_time;	}
	static bool sort_env_etl_pred	(const CEnvDescriptor* x, const CEnvDescriptor* y)
	{	return x->exec_time_loaded < y->exec_time_loaded;	}
protected:
	CPerlinNoise1D*			PerlinNoise1D;

	float					fGameTime;
public:
	FactoryPtr<IEnvironmentRender>	m_pRender;
	BOOL					bNeed_re_create_env;

	float					wind_strength_factor;	
	float					wind_gust_factor;

	// wind blast params
	float					wind_blast_strength;
	Fvector					wind_blast_direction;
	Fquaternion				wind_blast_start_time;
	Fquaternion				wind_blast_stop_time;
	float					wind_blast_strength_start_value;
	float					wind_blast_strength_stop_value;
	Fquaternion				wind_blast_current;
	// Environments
	BENCH_SEC_SCRAMBLEMEMBER2
	CEnvDescriptorMixer*	CurrentEnv;
	CEnvDescriptor*			Current[2];

	bool					bWFX;
	float					wfx_time;
	CEnvDescriptor*			WFX_end_desc[2];
    
    EnvVec*					CurrentWeather;
    shared_str				CurrentWeatherName;
	shared_str				CurrentCycleName;

	EnvsMap					WeatherCycles;
	EnvsMap					WeatherFXs;
	xr_vector<CEnvModifier>	Modifiers;
	EnvAmbVec				Ambients;

	CEffect_Rain*			eff_Rain;
	CLensFlare*				eff_LensFlare;
	CEffect_Thunderbolt*	eff_Thunderbolt;

	float					fTimeFactor;

    void					SelectEnvs			(float gt);

	void					UpdateAmbient		();
	INGAME_EDITOR_VIRTUAL CEnvAmbient* AppendEnvAmb	(const shared_str& sect);

	void					Invalidate			();
public:
							CEnvironment		();

	INGAME_EDITOR_VIRTUAL	~CEnvironment		();

	INGAME_EDITOR_VIRTUAL void	load			();
    INGAME_EDITOR_VIRTUAL void	unload			();

	void					mods_load			();
	void					mods_unload			();

	void					OnFrame				();
	void					lerp				(float& current_weight);

	void					RenderSky			();
	void					RenderClouds		();
	void					RenderFlares		();
	void					RenderLast			();

	bool					SetWeatherFX		(shared_str name);
	bool					StartWeatherFXFromTime	(shared_str name, float time);
	bool					IsWFXPlaying		(){return bWFX;}
	void					StopWFX				();

	void					SetWeather			(shared_str name, bool forced=false);
    shared_str				GetWeather			()					{ return CurrentWeatherName;}
	void					ChangeGameTime		(float game_time);
	void					SetGameTime			(float game_time, float time_factor);

	void					OnDeviceCreate		();
	void					OnDeviceDestroy		();

	// editor-related
#ifdef _EDITOR
public:
	float					ed_from_time		;
	float					ed_to_time			;
public:
    void					ED_Reload			();
    float					GetGameTime			(){return fGameTime;}
#else // #ifdef _EDITOR
#	ifdef INGAME_EDITOR
		float				GetGameTime			(){return fGameTime;}
#	endif // #ifdef INGAME_EDITOR

	bool					m_paused;
#endif // #ifdef _EDITOR

	CInifile*				m_ambients_config;
	CInifile*				m_sound_channels_config;
	CInifile*				m_effects_config;
	CInifile*				m_suns_config;
	CInifile*				m_thunderbolt_collections_config;
	CInifile*				m_thunderbolts_config;

protected:
	INGAME_EDITOR_VIRTUAL	CEnvDescriptor* create_descriptor	(shared_str const& identifier, CInifile* config);
	INGAME_EDITOR_VIRTUAL	void load_weathers					();
	INGAME_EDITOR_VIRTUAL	void load_weather_effects			();
	INGAME_EDITOR_VIRTUAL	void create_mixer					();
							void destroy_mixer					();

							void load_level_specific_ambients   ();

public:
	INGAME_EDITOR_VIRTUAL	SThunderboltDesc* thunderbolt_description		(CInifile& config, shared_str const& section);
	INGAME_EDITOR_VIRTUAL	SThunderboltCollection* thunderbolt_collection	(CInifile* pIni, CInifile* thunderbolts, LPCSTR section);
	INGAME_EDITOR_VIRTUAL	SThunderboltCollection* thunderbolt_collection	(xr_vector<SThunderboltCollection*>& collection,  shared_str const& id);
	INGAME_EDITOR_VIRTUAL	CLensFlareDescriptor*	add_flare				(xr_vector<CLensFlareDescriptor*>& collection, shared_str const& id);

public:
	float						p_var_alt;
	float						p_var_long;
	float						p_min_dist;
	float						p_tilt;
	float						p_second_prop;
	float						p_sky_color;
	float						p_sun_color;
	float						p_fog_color;
};

#undef INGAME_EDITOR_VIRTUAL

ENGINE_API extern Flags32	psEnvFlags;
ENGINE_API extern float		psVisDistance;

#endif //EnvironmentH