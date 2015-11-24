#pragma once

class CBlender_bloom_build : public IBlender  
{
public:
	virtual		LPCSTR		getComment()	{ return "INTERNAL: combine to bloom target";	}
	virtual		BOOL		canBeDetailed()	{ return FALSE;	}
	virtual		BOOL		canBeLMAPped()	{ return FALSE;	}

	virtual		void		Compile			(CBlender_Compile& C);

	CBlender_bloom_build();
	virtual ~CBlender_bloom_build();
};

class CBlender_bloom_build_msaa : public IBlender  
{
public:
   virtual		LPCSTR		getComment()	{ return "INTERNAL: combine to bloom target msaa";	}
   virtual		BOOL		canBeDetailed()	{ return FALSE;	}
   virtual		BOOL		canBeLMAPped()	{ return FALSE;	}

   virtual		void		Compile			(CBlender_Compile& C);

   CBlender_bloom_build_msaa();
   virtual ~CBlender_bloom_build_msaa();
};

class CBlender_postprocess_msaa : public IBlender  
{
public:
   virtual		LPCSTR		getComment()	{ return "INTERNAL: combine to bloom target msaa";	}
   virtual		BOOL		canBeDetailed()	{ return FALSE;	}
   virtual		BOOL		canBeLMAPped()	{ return FALSE;	}

   virtual		void		Compile			(CBlender_Compile& C);

   CBlender_postprocess_msaa();
   virtual ~CBlender_postprocess_msaa();
};

