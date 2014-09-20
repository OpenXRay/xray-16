#ifndef	dx103DFluidBlenders_included
#define	dx103DFluidBlenders_included
#pragma once

class CBlender_fluid_advect	: public IBlender  
{
public:
	virtual		LPCSTR		getComment()	{ return "INTERNAL: 3dfluid maths";	}
	virtual		BOOL		canBeDetailed()	{ return FALSE;	}
	virtual		BOOL		canBeLMAPped()	{ return FALSE;	}

	virtual		void		Compile			(CBlender_Compile& C);
};

class CBlender_fluid_advect_velocity	: public IBlender  
{
public:
	virtual		LPCSTR		getComment()	{ return "INTERNAL: 3dfluid maths";	}
	virtual		BOOL		canBeDetailed()	{ return FALSE;	}
	virtual		BOOL		canBeLMAPped()	{ return FALSE;	}

	virtual		void		Compile			(CBlender_Compile& C);
};

class CBlender_fluid_simulate	: public IBlender  
{
public:
	virtual		LPCSTR		getComment()	{ return "INTERNAL: 3dfluid maths";	}
	virtual		BOOL		canBeDetailed()	{ return FALSE;	}
	virtual		BOOL		canBeLMAPped()	{ return FALSE;	}

	virtual		void		Compile			(CBlender_Compile& C);
};

class CBlender_fluid_obst	: public IBlender  
{
public:
	virtual		LPCSTR		getComment()	{ return "INTERNAL: 3dfluid maths 2";	}
	virtual		BOOL		canBeDetailed()	{ return FALSE;	}
	virtual		BOOL		canBeLMAPped()	{ return FALSE;	}

	virtual		void		Compile			(CBlender_Compile& C);
};

class CBlender_fluid_emitter	: public IBlender  
{
public:
	virtual		LPCSTR		getComment()	{ return "INTERNAL: 3dfluid emitters";	}
	virtual		BOOL		canBeDetailed()	{ return FALSE;	}
	virtual		BOOL		canBeLMAPped()	{ return FALSE;	}

	virtual		void		Compile			(CBlender_Compile& C);
};

class CBlender_fluid_obstdraw	: public IBlender  
{
public:
	virtual		LPCSTR		getComment()	{ return "INTERNAL: 3dfluid maths 2";	}
	virtual		BOOL		canBeDetailed()	{ return FALSE;	}
	virtual		BOOL		canBeLMAPped()	{ return FALSE;	}

	virtual		void		Compile			(CBlender_Compile& C);
};

class CBlender_fluid_raydata	: public IBlender  
{
public:
	virtual		LPCSTR		getComment()	{ return "INTERNAL: 3dfluid maths 2";	}
	virtual		BOOL		canBeDetailed()	{ return FALSE;	}
	virtual		BOOL		canBeLMAPped()	{ return FALSE;	}

	virtual		void		Compile			(CBlender_Compile& C);
};

class CBlender_fluid_raycast	: public IBlender  
{
public:
	virtual		LPCSTR		getComment()	{ return "INTERNAL: 3dfluid maths 2";	}
	virtual		BOOL		canBeDetailed()	{ return FALSE;	}
	virtual		BOOL		canBeLMAPped()	{ return FALSE;	}

	virtual		void		Compile			(CBlender_Compile& C);
};

#endif	//	dx103DFluidBlenders_included