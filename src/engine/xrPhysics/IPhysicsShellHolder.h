#pragma once 
class ICollisionDamageInfo;
class IDamageSource;
//class IKinematics;
//class IRenderVisual;
class IKinematics;
class CPhysicsShell;
class IPHCapture;
class IPhysicsShellHolder;
class CPHSoundPlayer;
class ICollisionDamageReceiver;
class ICollisionForm;
class ICollisionHitCallback
{
public:
	virtual	void call( IPhysicsShellHolder* obj, float min_cs, float max_cs, float &cs,float &hl, ICollisionDamageInfo* di ) = 0;
	virtual	~ICollisionHitCallback() 	{}
};

#ifdef	DEBUG
	enum	EDumpType
	{
		base,
		poses,
		vis_geom,
		props,
		full,
		full_capped
	};
#endif

class IPhysicsShellHolder
{
	public:

	virtual	Fmatrix&					_BCL	ObjectXFORM						()						=0;
	virtual	Fvector&					_BCL	ObjectPosition						()						=0;
	virtual	LPCSTR						_BCL	ObjectName							()		const			=0;
	virtual	LPCSTR						_BCL	ObjectNameVisual					()		const			=0;
	virtual	LPCSTR						_BCL	ObjectNameSect						()		const			=0;
	virtual	bool						_BCL	ObjectGetDestroy					()		const			=0;
	virtual ICollisionHitCallback*		_BCL	ObjectGetCollisionHitCallback		()						=0;
	virtual	u16							_BCL	ObjectID							()		const			=0;
	virtual	ICollisionForm*				_BCL	ObjectCollisionModel				()						=0;
//	virtual	IRenderVisual*				_BCL	ObjectVisual						()						=0;
	virtual	IKinematics*				_BCL	ObjectKinematics					()						=0;
	virtual IDamageSource*				_BCL	ObjectCastIDamageSource				()						=0;
	virtual	void						_BCL	ObjectProcessingDeactivate			()						=0;
	virtual	void						_BCL	ObjectProcessingActivate			()						=0;				
	virtual	void						_BCL	ObjectSpatialMove					()						=0;
	virtual	CPhysicsShell*&				_BCL	ObjectPPhysicsShell				()						=0;
	virtual	void						_BCL	enable_notificate					()						=0;
	virtual bool						_BCL	has_parent_object					()						=0;
	virtual	void						_BCL	on_physics_disable					()						=0;
	virtual	IPHCapture*					_BCL	PHCapture							()						=0;
	virtual	bool						_BCL	IsInventoryItem						()						=0;
	virtual	bool						_BCL	IsActor								()						=0;
	virtual bool						_BCL	IsStalker							()						=0;
	//virtual	void							SetWeaponHideState					( u16 State, bool bSet )=0;
	virtual	void						_BCL	HideAllWeapons						( bool v )				=0;//(SetWeaponHideState(INV_STATE_BLOCK_ALL,true))
	virtual	void						_BCL	MovementCollisionEnable				( bool enable )			=0;
	virtual CPHSoundPlayer*				_BCL	ObjectPhSoundPlayer				()  					=0;
	virtual	ICollisionDamageReceiver*	_BCL	ObjectPhCollisionDamageReceiver	()						=0;
	virtual	void						_BCL BonceDamagerCallback				( float &damage_factor )=0;
#ifdef	DEBUG
	virtual	std::string					_BCL	dump(EDumpType type) const =0;
#endif
};

