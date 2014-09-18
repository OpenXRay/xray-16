#ifndef EnvelopeH
#define EnvelopeH

// refs
class IWriter;
class IReader;

/*
======================================================================
envelope.h

  Definitions for local copies of LightWave envelopes.

	Ernie Wright  31 Aug 00

	  The LightWave plug-in SDK provides its own representation of LightWave
	  envelopes that plug-ins can use.  These definitions are for standalone
	  programs that, for example, read scene or object files and must store
	  the envelopes.
====================================================================== */

#define SHAPE_TCB   0
#define SHAPE_HERM  1
#define SHAPE_BEZI  2
#define SHAPE_LINE  3
#define SHAPE_STEP  4
#define SHAPE_BEZ2  5

#define BEH_RESET      0
#define BEH_CONSTANT   1
#define BEH_REPEAT     2
#define BEH_OSCILLATE  3
#define BEH_OFFSET     4
#define BEH_LINEAR     5


#pragma pack( push,1 )
struct st_Key{
	enum{
    	ktStepped = 1<<0,
    };
	float		value;
	float		time;
	u8			shape;
	float		tension;
	float		continuity;
	float		bias;
	float		param[ 4 ];
				st_Key		(){ZeroMemory(this,sizeof(st_Key));}
    IC bool		equal		(const st_Key& tgt)
    {
    	if (!fsimilar(value,tgt.value)) 			return false;
    	if (!fsimilar(shape,tgt.shape)) 			return false;
    	if (!fsimilar(tension,tgt.tension)) 		return false;
    	if (!fsimilar(continuity,tgt.continuity)) 	return false;
    	if (!fsimilar(bias,tgt.bias)) 				return false;
    	if (!fsimilar(param[0],tgt.param[0])) 		return false;
    	if (!fsimilar(param[1],tgt.param[1]))	 	return false;
    	if (!fsimilar(param[2],tgt.param[2])) 		return false;
    	if (!fsimilar(param[3],tgt.param[3])) 		return false;
        return true;
    }
    IC void		Save		(IWriter& F)
    {
        F.w_float	(value);
        F.w_float	(time);
        F.w_u8		(shape);
        if (shape!=4){ // ! Stepped
            F.w_float_q16(tension,-32.f,32.f);
            F.w_float_q16(continuity,-32.f,32.f);
            F.w_float_q16(bias,-32.f,32.f);
            F.w_float_q16(param[0],-32.f,32.f);
            F.w_float_q16(param[1],-32.f,32.f);
            F.w_float_q16(param[2],-32.f,32.f);
            F.w_float_q16(param[3],-32.f,32.f);
        }
    }
	IC void		Load_1		(IReader& F)
    {
        value		= F.r_float();
        time		= F.r_float();
        shape		= u8((u8)F.r_u32()&0xff);
        tension		= F.r_float();
        continuity	= F.r_float();
        bias		= F.r_float();
        F.r			(&param,sizeof(float)*4);
    }
	IC void		Load_2		(IReader& F)
    {
        value		= F.r_float();
        time		= F.r_float();
        shape		= F.r_u8();
        if (shape!=4){ // ! Stepped
            tension		= F.r_float_q16(-32.f,32.f);
            continuity	= F.r_float_q16(-32.f,32.f);
            bias		= F.r_float_q16(-32.f,32.f);
            param[0]	= F.r_float_q16(-32.f,32.f);
            param[1]	= F.r_float_q16(-32.f,32.f);
            param[2]	= F.r_float_q16(-32.f,32.f);
            param[3]	= F.r_float_q16(-32.f,32.f);
        }
    }
};
#pragma pack( pop )

DEFINE_VECTOR(st_Key*,KeyVec,KeyIt);

// refs
class CExporter;

class ENGINE_API CEnvelope {
public:
	KeyVec		keys;
	int			behavior[2];
public:
				CEnvelope	(){behavior[0]=1;behavior[1]=1;}
				CEnvelope	(CEnvelope* source);
	virtual		~CEnvelope	();

	float		Evaluate	(float t);

	void		Clear		();
	void		ClearAndFree();
	void		Save		(IWriter& F);
	void		Load_1		(IReader& F);
	void		Load_2		(IReader& F);
	void		SaveA		(IWriter& F);
	void		LoadA		(IReader& F);

    void		RotateKeys	(float angle);

    KeyIt		FindKey		(float t, float eps);
    void		FindNearestKey(float t, KeyIt& min, KeyIt& max, float eps);
    void		InsertKey	(float t, float val);                                   
    void		DeleteKey	(float t);
    BOOL		ScaleKeys	(float from_time, float to_time, float scale_factor, float eps);
    float		GetLength	(float* mn, float* mx);

    void		Optimize	();
};

#endif //_ENVELOPE_H_
