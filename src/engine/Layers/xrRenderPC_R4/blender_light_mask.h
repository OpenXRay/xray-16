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

class CBlender_accum_direct_mask_msaa : public IBlender  
{
public:

	virtual		LPCSTR	getComment()	{ return "INTERNAL: mask direct light msaa";	}
	virtual		BOOL		canBeDetailed()	{ return FALSE;	}
	virtual		BOOL		canBeLMAPped()	{ return FALSE;	}
  virtual  void     SetDefine( LPCSTR Name, LPCSTR Definition )
  {
      this->Name = Name;
      this->Definition = Definition;
  }

	virtual		void		Compile			(CBlender_Compile& C);

	CBlender_accum_direct_mask_msaa();
	virtual ~CBlender_accum_direct_mask_msaa();

  LPCSTR Name;
  LPCSTR Definition;
};

