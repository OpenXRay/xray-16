#pragma once
 

class IDamageSource
{
public:
	virtual							~IDamageSource			()												{}				;
	virtual			void			SetInitiator			(u16 id)										=0				;
	virtual			u16				Initiator				()												=0				;
	virtual			IDamageSource	*cast_IDamageSource		()												=0				;//{return this	;}
};