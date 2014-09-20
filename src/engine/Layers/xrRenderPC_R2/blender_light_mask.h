#pragma once

class CBlender_accum_direct_mask : public IBlender  
{
public:
	virtual		LPCSTR		getComment()	{ return "INTERNAL: mask direct light";	}
	virtual		BOOL		canBeDetailed()	{ return FALSE;	}
	virtual		BOOL		canBeLMAPped()	{ return FALSE;	}

	virtual		void		Compile			(CBlender_Compile& C);

	CBlender_accum_direct_mask();
	virtual ~CBlender_accum_direct_mask();
};
