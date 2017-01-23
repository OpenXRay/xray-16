#pragma once

class CBlender_SSAO : public IBlender  
{
public:
	virtual		LPCSTR		getComment()	{ return "INTERNAL: calc SSAO";	}
	virtual		BOOL		canBeDetailed()	{ return FALSE;	}
	virtual		BOOL		canBeLMAPped()	{ return FALSE;	}

	virtual		void		Compile			(CBlender_Compile& C);

	CBlender_SSAO();
	virtual ~CBlender_SSAO();
};
