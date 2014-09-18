//---------------------------------------------------------------------------
#ifndef ParticleEffectActionsH
#define ParticleEffectActionsH

struct PBool{
	BOOL		val;
    PBool		():val(FALSE){}
    PBool		(BOOL _val):val(_val){}
    void 		set(BOOL v){val=v;}
};
struct PFloat{
    float		val;
    float		mn;
    float		mx;
    PFloat		()	{val=0.f;mn=0.f;mx=0.f;}
    PFloat		(float _val, float _mn, float _mx):val(_val),mn(_mn),mx(_mx){}
    void 		set(float v){val=v;}
};
struct PInt{
    int			val;
    int			mn;
    int			mx;
    PInt		()	{val=0;mn=0;mx=0;}
    PInt		(int _val, int _mn, int _mx):val(_val),mn(_mn),mx(_mx){}
    void 		set(int v){val=v;}
};
struct PVector{
    Fvector		val;
    float		mn;
    float		mx;
    enum EType{
        vNum,
        vAngle,
        vColor,
        _force_u32 = u32(-1),
    };
    EType		type;
    PVector		(){val.set(0,0,0);mn=0.f;mx=0.f;}
    PVector		(EType t, Fvector _val, float _mn, float _mx):type(t),val(_val),mn(_mn),mx(_mx){}
    void 		set(const Fvector& v){val.set(v);}
    void 		set(float x, float y, float z){val.set(x,y,z);}
};
struct PDomain
{
public:
	PAPI::PDomainEnum	type;
    union{
		float	f[9];
        Fvector	v[3];  
    };
    enum EType{
        vNum,
        vAngle,
        vColor,
        _force_u32 = u32(-1),
    };
    enum {
    	flRenderable = (1<<0)
    };
    EType		e_type;
    Flags32		flags;
    u32			clr;
protected:
	void __stdcall PDomain::OnTypeChange(PropValue* sender);
public:
	PDomain 	(){}
	PDomain		(EType et, BOOL renderable, u32 color=0x00000000, PAPI::PDomainEnum type = PAPI::PDPoint,	
    												float inA0 = 0.0f,	float inA1 = 0.0f,	float inA2 = 0.0f,
													float inA3 = 0.0f,	float inA4 = 0.0f,	float inA5 = 0.0f,
													float inA6 = 0.0f,	float inA7 = 0.0f,	float inA8 = 0.0f	);
	~PDomain	();
	PDomain 	(const PDomain &in);

	void		MoveXYZ		(float x, float y, float z);
	void		RotateXYZ	(float x, float y, float z);
	void		ScaleXYZ	(float x, float y, float z);

	Fvector&	GetCenter();
	
    void 		Load		(IReader& F);
    void 		Save		(IWriter& F);

	void 		Load2		(CInifile& ini, const shared_str& sect);
    void 		Save2		(CInifile& ini, const shared_str& sect);
    
	void 		Render		(u32 color, const Fmatrix& parent);
    void 		FillProp	(PropItemVec& items, LPCSTR pref, u32 clr);
};
struct EParticleAction
{
	DEFINE_MAP(AnsiString,PDomain,	PDomainMap,	PDomainMapIt);
	DEFINE_MAP(AnsiString,PBool,	PBoolMap,	PBoolMapIt);
	DEFINE_MAP(AnsiString,PFloat,	PFloatMap,	PFloatMapIt);
    DEFINE_MAP(AnsiString,PInt,		PIntMap,	PIntMapIt);
	DEFINE_MAP(AnsiString,PVector,	PVectorMap,	PVectorMapIt);

    shared_str 		actionName;
	shared_str 		actionType;
    shared_str		hint;
    enum{
    	flEnabled	= (1<<0),
    	flDraw		= (1<<1),
    };
	Flags32				flags;
    PAPI::PActionEnum	type;

	PDomainMap		domains;
	PBoolMap		bools;
	PFloatMap		floats;
    PIntMap			ints;
	PVectorMap		vectors;

    enum EValType{
    	tpDomain,
        tpVector,
        tpFloat,
        tpBool,
        tpInt,
    };
    struct SOrder{
    	EValType	type;
    	AnsiString	name;
        SOrder(EValType	_type, AnsiString _name):type(_type),name(_name){}
    };
	DEFINE_VECTOR	(SOrder, OrderVec,	OrderVecIt);
    OrderVec		orders;
    
    EParticleAction	(PAPI::PActionEnum	_type)
    {
    	flags.assign(flEnabled);
        type		= _type;
    }
public:
	void			appendFloat	(LPCSTR name, float v, float mn, float mx);
	void			appendInt	(LPCSTR name, int v, int mn=-P_MAXINT, int mx=P_MAXINT);
	void			appendVector(LPCSTR name, PVector::EType type, float vx, float vy, float vz, float mn=-P_MAXFLOAT, float mx=P_MAXFLOAT);
	void			appendDomain(LPCSTR name, PDomain v);
	void			appendBool	(LPCSTR name, BOOL b);
	PFloat&			_float		(LPCSTR name){PFloatMapIt 	it=floats.find(name); 	R_ASSERT2(it!=floats.end(),name);	return it->second;}
	PInt&			_int		(LPCSTR name){PIntMapIt 	it=ints.find(name); 	R_ASSERT2(it!=ints.end(),name);		return it->second;}
	PVector&		_vector		(LPCSTR name){PVectorMapIt 	it=vectors.find(name); 	R_ASSERT2(it!=vectors.end(),name);	return it->second;}
	PDomain&		_domain		(LPCSTR name){PDomainMapIt 	it=domains.find(name); 	R_ASSERT2(it!=domains.end(),name);	return it->second;}
	PBool&			_bool		(LPCSTR name){PBoolMapIt 	it=bools.find(name); 	R_ASSERT2(it!=bools.end(),name); 	return it->second;}
	PBool*			_bool_safe	(LPCSTR name){PBoolMapIt 	it=bools.find(name); 	return (it!=bools.end())?&it->second:0;}
public:    
    virtual void	Compile		(IWriter& F)=0;
    virtual void 	FillProp	(PropItemVec& items, LPCSTR pref, u32 clr);

    virtual void 	Load		(IReader& F);
    virtual void 	Save		(IWriter& F);
    virtual void 	Load2		(CInifile& ini, const shared_str& sect);
    virtual void 	Save2		(CInifile& ini, const shared_str& sect);
    virtual void 	Render		(const Fmatrix& parent);
};

struct EPAAvoid : public EParticleAction
{
					EPAAvoid	();
    virtual void	Compile		(IWriter& F);
};
 
struct EPABounce : public EParticleAction
{
					EPABounce	();
    virtual void	Compile		(IWriter& F);
};

struct EPACopyVertexB : public EParticleAction
{
					EPACopyVertexB();
    virtual void	Compile		(IWriter& F);
};

struct EPADamping : public EParticleAction
{
					EPADamping	();
    virtual void	Compile		(IWriter& F);
};

struct EPAExplosion : public EParticleAction
{
					EPAExplosion();
    virtual void	Compile		(IWriter& F);
};

struct EPAFollow : public EParticleAction
{
					EPAFollow	();
    virtual void	Compile		(IWriter& F);
};

struct EPAGravitate : public EParticleAction
{
					EPAGravitate();
    virtual void	Compile		(IWriter& F);
};

struct EPAGravity : public EParticleAction
{
					EPAGravity	();
    virtual void	Compile		(IWriter& F);
};

struct EPAJet : public EParticleAction
{
					EPAJet		();
    virtual void	Compile		(IWriter& F);
    virtual void 	Render		(const Fmatrix& parent);
};

struct EPAKillOld : public EParticleAction
{
					EPAKillOld	();
    virtual void	Compile		(IWriter& F);
};

struct EPAMatchVelocity : public EParticleAction
{
					EPAMatchVelocity();
    virtual void	Compile		(IWriter& F);
};

struct EPAMove : public EParticleAction
{
					EPAMove		();
    virtual void	Compile		(IWriter& F);
};

struct EPAOrbitLine : public EParticleAction
{
					EPAOrbitLine();
    virtual void	Compile		(IWriter& F);
    virtual void 	Render		(const Fmatrix& parent);
};

struct EPAOrbitPoint : public EParticleAction
{
					EPAOrbitPoint();
    virtual void	Compile		(IWriter& F);
    virtual void 	Render		(const Fmatrix& parent);
};

struct EPARandomAccel : public EParticleAction
{
					EPARandomAccel();
    virtual void	Compile		(IWriter& F);
};

struct EPARandomDisplace : public EParticleAction
{
					EPARandomDisplace();
    virtual void	Compile		(IWriter& F);
};

struct EPARandomVelocity : public EParticleAction
{
					EPARandomVelocity();
    virtual void	Compile		(IWriter& F);
};

struct EPARestore : public EParticleAction
{
					EPARestore	();
    virtual void	Compile		(IWriter& F);
};

struct EPAScatter : public EParticleAction
{
					EPAScatter	();
    virtual void	Compile		(IWriter& F);
    virtual void 	Render		(const Fmatrix& parent);
};

struct EPASink : public EParticleAction
{
					EPASink		();
    virtual void	Compile		(IWriter& F);
};

struct EPASinkVelocity : public EParticleAction
{
					EPASinkVelocity();
    virtual void	Compile		(IWriter& F);
};

struct EPASpeedLimit : public EParticleAction
{
					EPASpeedLimit();
    virtual void	Compile		(IWriter& F);
};

struct EPASource : public EParticleAction
{
					EPASource	();
    virtual void	Compile		(IWriter& F);
};

struct EPATargetColor : public EParticleAction
{
					EPATargetColor();
    virtual void	Compile		(IWriter& F);
};

struct EPATargetSize : public EParticleAction
{
					EPATargetSize();
    virtual void	Compile		(IWriter& F);
};

struct EPATargetRotate : public EParticleAction
{
					EPATargetRotate();
    virtual void	Compile		(IWriter& F);
};

struct EPATargetVelocity : public EParticleAction
{
					EPATargetVelocity();
    virtual void	Compile		(IWriter& F);
};

struct EPAVortex : public EParticleAction
{
					EPAVortex	();
    virtual void	Compile		(IWriter& F);
    virtual void 	Render		(const Fmatrix& parent);
};

struct EPATurbulence : public EParticleAction
{
	float	***nval;
    float 	age;
public:
					EPATurbulence();
	
    virtual void	Compile		(IWriter& F);
    virtual void 	Render		(const Fmatrix& parent);
};

extern xr_token2	actions_token[];

typedef EParticleAction* (*_CreateEAction)(PAPI::PActionEnum type);
extern ECORE_API _CreateEAction 	pCreateEAction;
//---------------------------------------------------------------------------
#endif

/*

	 void pTargetVelocityD(float scale, PDomainEnum dtype,
		float a0 = 0.0f, float a1 = 0.0f, float a2 = 0.0f,
		float a3 = 0.0f, float a4 = 0.0f, float a5 = 0.0f,
		float a6 = 0.0f, float a7 = 0.0f, float a8 = 0.0f, BOOL allow_translate=TRUE, BOOL allow_rotate=TRUE);

	 void pVertex(float x, float y, float z);

}
*/


