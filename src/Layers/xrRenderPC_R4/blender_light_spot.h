#pragma once

class CBlender_accum_spot : public IBlender  
{
public:
	virtual		LPCSTR	getComment()	{ return "INTERNAL: accumulate spot light";	}
	virtual		BOOL		canBeDetailed()	{ return FALSE;	}
	virtual		BOOL		canBeLMAPped()	{ return FALSE;	}

	virtual		void		Compile			(CBlender_Compile& C);

	CBlender_accum_spot();
	virtual ~CBlender_accum_spot();
};

class CBlender_accum_spot_msaa : public IBlender  
{
public:
	virtual		LPCSTR	getComment()	{ return "INTERNAL: accumulate spot light msaa";	}
	virtual		BOOL		canBeDetailed()	{ return FALSE;	}
	virtual		BOOL		canBeLMAPped()	{ return FALSE;	}

	virtual		void		Compile			(CBlender_Compile& C);
	
	virtual   void    SetDefine( LPCSTR Name, LPCSTR Definition )
	{
		this->Name = Name;
		this->Definition = Definition;
	}

	CBlender_accum_spot_msaa();
	virtual ~CBlender_accum_spot_msaa();
	LPCSTR Name;
	LPCSTR Definition;
};

class CBlender_accum_volumetric_msaa : public IBlender  
{
public:
	virtual		LPCSTR	getComment()	{ return "INTERNAL: accumulate spot light msaa";	}
	virtual		BOOL		canBeDetailed()	{ return FALSE;	}
	virtual		BOOL		canBeLMAPped()	{ return FALSE;	}

	virtual		void		Compile			(CBlender_Compile& C);
	
	virtual   void    SetDefine( LPCSTR Name, LPCSTR Definition )
	{
		this->Name = Name;
		this->Definition = Definition;
	}

	CBlender_accum_volumetric_msaa();
	virtual ~CBlender_accum_volumetric_msaa();
	LPCSTR Name;
	LPCSTR Definition;
};

