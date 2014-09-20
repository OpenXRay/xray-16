#pragma once

class CBlender_accum_direct : public IBlender  
{
public:
	virtual		LPCSTR	getComment()	{ return "INTERNAL: accumulate direct light";	}
	virtual		BOOL		canBeDetailed()	{ return FALSE;	}
	virtual		BOOL		canBeLMAPped()	{ return FALSE;	}

	virtual		void		Compile			(CBlender_Compile& C);

	CBlender_accum_direct();
	virtual ~CBlender_accum_direct();
};

class CBlender_accum_direct_msaa : public IBlender  
{
public:

  virtual		LPCSTR	getComment()	{ return "INTERNAL: accumulate direct light";	}
  virtual		BOOL		canBeDetailed()	{ return FALSE;	}
  virtual		BOOL		canBeLMAPped()	{ return FALSE;	}

  virtual		void		Compile			(CBlender_Compile& C);
  virtual   void    SetDefine( LPCSTR Name, LPCSTR Definition )
  {
     this->Name = Name;
     this->Definition = Definition;
  }
   LPCSTR Name;
	LPCSTR Definition;
	
	CBlender_accum_direct_msaa();
	virtual ~CBlender_accum_direct_msaa();

};

class CBlender_accum_direct_volumetric_msaa : public IBlender  
	{
	public:

		virtual		LPCSTR	getComment()	{ return "INTERNAL: accumulate direct light";	}
		virtual		BOOL		canBeDetailed()	{ return FALSE;	}
		virtual		BOOL		canBeLMAPped()	{ return FALSE;	}

		virtual		void		Compile			(CBlender_Compile& C);
		virtual   void    SetDefine( LPCSTR Name, LPCSTR Definition )
		{
			this->Name = Name;
			this->Definition = Definition;
		}

		CBlender_accum_direct_volumetric_msaa();
		virtual ~CBlender_accum_direct_volumetric_msaa();
		LPCSTR Name;
		LPCSTR Definition;
	};

class CBlender_accum_direct_volumetric_sun_msaa : public IBlender  
	{
	public:

		virtual		LPCSTR	getComment()	{ return "INTERNAL: accumulate direct light";	}
		virtual		BOOL		canBeDetailed()	{ return FALSE;	}
		virtual		BOOL		canBeLMAPped()	{ return FALSE;	}

		virtual		void		Compile			(CBlender_Compile& C);
		virtual   void    SetDefine( LPCSTR Name, LPCSTR Definition )
		{
			this->Name = Name;
			this->Definition = Definition;
		}

		CBlender_accum_direct_volumetric_sun_msaa();
		virtual ~CBlender_accum_direct_volumetric_sun_msaa();
		LPCSTR Name;
		LPCSTR Definition;
	};
