#pragma once

class CBlender_deffer_aref : public IBlender  
{
public:
	xrP_Integer	oAREF;
	xrP_BOOL	oBlend;
	bool		lmapped;
public:
	virtual		LPCSTR		getComment()	{ return "LEVEL: defer-base-aref";	}
	virtual		BOOL		canBeDetailed()	{ return TRUE;		}
	virtual		BOOL		canBeLMAPped()	{ return lmapped;	}
	virtual		BOOL		canUseSteepParallax	()	{ return TRUE; }

	virtual		void		Save			(IWriter&	fs);
	virtual		void		Load			(IReader&	fs, u16 version);
	virtual		void		Compile			(CBlender_Compile& C);

	CBlender_deffer_aref					(bool _lmapped=false);
	virtual ~CBlender_deffer_aref			();
};
