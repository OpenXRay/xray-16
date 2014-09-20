
#pragma once

#include "CameraDefs.h"


class ENGINE_API CEffectorCam :public SBaseEffector
{
protected:
	ECamEffectorType	eType;
	
	friend class		CCameraManager;
	float				fLifeTime;
	bool				bHudAffect;
public:
						CEffectorCam	(ECamEffectorType type, float tm)	{eType=type; fLifeTime=tm; bHudAffect=true;};
						CEffectorCam	()									{eType=(ECamEffectorType)0; fLifeTime=0.0f; bHudAffect=true;};
	virtual				~CEffectorCam	()									{};
			void		SetType			(ECamEffectorType type)				{eType=type;}
			void		SetHudAffect	(bool val)							{bHudAffect=val;}
			bool		GetHudAffect	()									{return bHudAffect;}

	IC ECamEffectorType	GetType			()									{return eType;}
	virtual	BOOL		Valid			()									{return fLifeTime>0.0f;}

	BENCH_SEC_SCRAMBLEVTBL1

	virtual BOOL		ProcessCam		(SCamEffectorInfo& info)			{fLifeTime-=Device.fTimeDelta; return Valid();};;
		
	virtual	void		ProcessIfInvalid(SCamEffectorInfo& info)			{};
	virtual BOOL		AllowProcessingIfInvalid()							{return FALSE;}
	virtual bool		AbsolutePositioning()								{return false;}		
};
