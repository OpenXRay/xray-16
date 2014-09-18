#ifndef I_COLLISION_DAMAGE_INFO_H
#define I_COLLISION_DAMAGE_INFO_H
//struct SCollisionHitCallback;
class ICollisionHitCallback;
class ICollisionDamageInfo
{
public:		
		virtual float					ContactVelocity				()				const						=0;
		virtual void					HitDir						(Fvector &dir)	const						=0;
		virtual const	Fvector&		HitPos						()				const						=0;
		virtual u16						DamageInitiatorID			()				const						=0;
		virtual CObject					*DamageInitiator			()				const						=0;
		virtual ALife::EHitType			HitType						()				const						=0;
		virtual ICollisionHitCallback	*HitCallback				()				const						=0;
		virtual void					Reinit						()											=0;
		virtual void					SetInitiated				()											=0;
		virtual bool					IsInitiated					()				const						=0;
		virtual bool					GetAndResetInitiated		()											=0;
protected:
		virtual							~ICollisionDamageInfo		() = 0	{}
};
#endif