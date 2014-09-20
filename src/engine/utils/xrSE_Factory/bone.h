//----------------------------------------------------
#ifndef BoneH
#define BoneH

#ifdef _LW_EXPORT
#include <lwrender.h>
#include <lwhost.h>
#endif

// refs
class CBone;

const	u16		BI_NONE				=	u16(-1);

#define OGF_IKDATA_VERSION		0x0001

#pragma pack( push,1 )
enum EJointType
{
    jtRigid,
	jtCloth,
	jtJoint,
    jtWheel,
    jtNone,
    jtSlider,
    jtForceU32 = u32(-1)
};

struct ECORE_API SJointLimit
{
	Fvector2		limit;
    float 			spring_factor;
    float 			damping_factor;
    SJointLimit		(){Reset();}
    void 			Reset()
    {
    	limit.set		(0.f,0.f);
        spring_factor 	= 1.f;
        damping_factor  = 1.f;
    }
};

struct ECORE_API SBoneShape
{
    enum EShapeType
	{
    	stNone,
        stBox,
        stSphere,
        stCylinder,
        stForceU32 = u16(-1)
    };

	enum EShapeFlags{
		sfNoPickable		= (1<<0), 	// use only in RayPick
        sfRemoveAfterBreak  = (1<<1),	
		sfNoPhysics			= (1<<2),	
	};

	u16				type;		// 2
	Flags16			flags;		// 2
  	Fobb			box;      	// 15*4
    Fsphere			sphere;		// 4*4
    Fcylinder		cylinder;	// 8*4
    SBoneShape		(){Reset();}
    void			Reset()
    {
		flags.zero	();
    	type		= stNone;
        box.invalidate();
        sphere.P.set(0.f,0.f,0.f); sphere.R = 0.f;
        cylinder.invalidate();
    }
    bool			Valid(){  
    	switch (type){
        case stBox: 	return !fis_zero(box.m_halfsize.x)&&!fis_zero(box.m_halfsize.x)&&!fis_zero(box.m_halfsize.x);
        case stSphere: 	return !fis_zero(sphere.R);
        case stCylinder:return !fis_zero(cylinder.m_height)&&!fis_zero(cylinder.m_radius)&&!fis_zero(cylinder.m_direction.square_magnitude());
        };
        return true;
    }
};

struct ECORE_API SJointIKData
{
    // IK
    EJointType		type;
    SJointLimit		limits	[3];// by [axis XYZ on joint] and[Z-wheel,X-steer on wheel]
    float			spring_factor;
    float			damping_factor;
    enum{
    	flBreakable	= (1<<0),
    };
    Flags32			ik_flags;
    float			break_force;	// [0..+INF]
    float			break_torque;	// [0..+INF]

    float			friction;
    			
    SJointIKData	(){ Reset();}
    void			Reset	()
    {
    	limits[0].Reset	();
    	limits[1].Reset	();
    	limits[2].Reset	();
        type			= jtRigid;
        spring_factor	= 1.f;
        damping_factor	= 1.f;
        ik_flags.zero	();
    	break_force		= 0.f;
    	break_torque	= 0.f;
    }
    void				clamp_by_limits(Fvector& dest_xyz);
    void				Export(IWriter& F)
    {
        F.w_u32			(type);
        for (int k=0; k<3; k++){
        	// Kostya Slipchenko say:
            // направление вращения в ОДЕ отличается от направления вращение в X-Ray 
            // поэтому меняем знак у лимитов
            F.w_float	(_min(-limits[k].limit.x,-limits[k].limit.y)); // min (swap special for ODE) 
            F.w_float	(_max(-limits[k].limit.x,-limits[k].limit.y)); // max (swap special for ODE)
            F.w_float	(limits[k].spring_factor);
            F.w_float	(limits[k].damping_factor);
        }
        F.w_float		(spring_factor);
        F.w_float		(damping_factor);

        F.w_u32			(ik_flags.get());
        F.w_float		(break_force);
        F.w_float		(break_torque);

        F.w_float		(friction);
    }
    bool				Import(IReader& F, u16 vers)
    {
        type			= (EJointType)F.r_u32();
        F.r				(limits,sizeof(SJointLimit)*3);
        spring_factor	= F.r_float();
        damping_factor	= F.r_float();
        ik_flags.flags	= F.r_u32();
        break_force		= F.r_float();
        break_torque	= F.r_float();
        if (vers>0){
	        friction	= F.r_float();
        }
        return true;
    }
};
#pragma pack( pop )

// refs
class CBone;
DEFINE_VECTOR		    (CBone*,BoneVec,BoneIt);

class ECORE_API CBone
{
	shared_str			name;
	shared_str			parent_name;
	shared_str			wmap;
	Fvector			    rest_offset;
	Fvector			    rest_rotate;    // XYZ format (Game format)
	float			    rest_length;

	Fvector			    mot_offset;
	Fvector			    mot_rotate;		// XYZ format (Game format)
	float			    mot_length;

    Fmatrix			    mot_transform;

    Fmatrix			    rest_transform;
    Fmatrix			    rest_i_transform;

    Fmatrix			    last_transform;

    Fmatrix				render_transform;
public:
	int				    SelfID;
    CBone*			    parent;
    BoneVec				children;
public:
    // editor part
    Flags8			    flags;    
	enum{
    	flSelected	    = (1<<0),
    };
    SJointIKData	    IK_data;
    shared_str			   game_mtl;
    SBoneShape		    shape;

    float			    mass;
    Fvector			    center_of_mass;
public:
					    CBone			();
	virtual			    ~CBone			();

	void			    SetName			(const char* p){name		= p; xr_strlwr(name);		}
	void			    SetParentName	(const char* p){parent_name	= p; xr_strlwr(parent_name);}
	void			    SetWMap			(const char* p){wmap		= p;}
	void			    SetRestParams	(float length, const Fvector& offset, const Fvector& rotate){rest_offset.set(offset);rest_rotate.set(rotate);rest_length=length;};

	shared_str		    Name			(){return name;}
	shared_str		    ParentName		(){return parent_name;}
	shared_str		    WMap			(){return wmap;}
	IC CBone*		    Parent			(){return parent;}
    IC BOOL			    IsRoot			(){return (parent==0);}

    // transformation
    const Fvector&      _Offset			(){return mot_offset;}
    const Fvector&      _Rotate			(){return mot_rotate;}
    float			    _Length			(){return mot_length;}
    IC Fmatrix&		    _RTransform		(){return rest_transform;}
    IC Fmatrix&		    _RITransform	(){return rest_i_transform;}
    IC Fmatrix&		    _MTransform		(){return mot_transform;}
    IC Fmatrix&		    _LTransform		(){return last_transform;}
    IC Fmatrix&		    _RenderTransform(){return render_transform;}
	IC Fvector&			_RestOffset		(){return rest_offset;}
	IC Fvector&		    _RestRotate		(){return rest_rotate;}
    
	void			    _Update			(const Fvector& T, const Fvector& R){mot_offset.set(T); mot_rotate.set(R); mot_length=rest_length;}
    void			    Reset			(){mot_offset.set(rest_offset); mot_rotate.set(rest_rotate); mot_length=rest_length;}

    // IO
	void			    Save			(IWriter& F);
	void			    Load_0			(IReader& F);
	void			    Load_1			(IReader& F);

#ifdef _LW_EXPORT
	void			    ParseBone		(LWItemID bone);
#endif

	void			    SaveData		(IWriter& F);
	void			    LoadData		(IReader& F);
    void			    ResetData		();
    void			    CopyData		(CBone* bone);
    
#ifdef _EDITOR
	void			    ShapeScale		(const Fvector& amount);
	void			    ShapeRotate		(const Fvector& amount);
	void			    ShapeMove		(const Fvector& amount);
	void			    BindRotate		(const Fvector& amount);
	void			    BindMove		(const Fvector& amount);
	void			    BoneMove		(const Fvector& amount);
	void			    BoneRotate		(const Fvector& axis, float angle);

	bool 			    Pick			(float& dist, const Fvector& S, const Fvector& D, const Fmatrix& parent);

    void			    Select			(BOOL flag)	{ flags.set(flSelected,flag); }
    bool			    Selected		(){return flags.is(flSelected);}

    void			    ClampByLimits	();

    bool 			    ExportOGF		(IWriter& F);
#endif
};

#endif
