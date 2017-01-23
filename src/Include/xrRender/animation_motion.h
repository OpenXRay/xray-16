#pragma once

struct MotionID {
private:
	typedef const MotionID *(MotionID::*unspecified_bool_type) () const;
public:
	union{
		struct{
			u16		idx:16; 	//14
			u16		slot:16;    //2
		};
//.		u16			val;
		u32			val;
	};
public:
	MotionID	(){invalidate();}
	MotionID	(u16 motion_slot, u16 motion_idx){set(motion_slot,motion_idx);}
	ICF bool		operator==	(const MotionID& tgt) const {return tgt.val==val;}
	ICF bool		operator!=	(const MotionID& tgt) const {return tgt.val!=val;}
	ICF bool		operator<	(const MotionID& tgt) const {return val<tgt.val;}
	ICF	bool		operator!	() const					{return !valid();}
	ICF void		set			(u16 motion_slot, u16 motion_idx){slot=motion_slot; idx=motion_idx;}
	ICF void		invalidate	() {val=u16(-1);}
	ICF bool		valid		() const {return val!=u16(-1);}
	const MotionID*	get			() const {return this;};
	ICF	operator	unspecified_bool_type () const 
	{
		if (valid())	return &MotionID::get;
		else			return 0;
		//		return(!valid()?0:&MotionID::get);
	}
};
