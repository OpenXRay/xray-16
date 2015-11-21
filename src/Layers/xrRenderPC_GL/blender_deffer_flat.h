#pragma once

class CBlender_deffer_flat : public IBlender  
{
public:
	virtual		LPCSTR		getComment()	{ return "LEVEL: defer-base-normal";	}
	virtual		BOOL		canBeDetailed()	{ return TRUE;	}
	virtual		BOOL		canBeLMAPped()	{ return FALSE;	}
	virtual		BOOL		canUseSteepParallax	()	{ return TRUE; }

	virtual		void		Save			(IWriter&	fs);
	virtual		void		Load			(IReader&	fs, u16 version);
	virtual		void		Compile			(CBlender_Compile& C);

	CBlender_deffer_flat();
	virtual ~CBlender_deffer_flat();
};
