#include "stdafx.h"
#include "DisablingParams.h"



SAllDDWParams	worldDisablingParams	=	
	{
	//object
		{
			{0.001f	,	0.1f	}		,	//translational		vel	,	accel
			{0.005f	,	0.05f	}		,	//rotational		vel	,	accel
			64								//level2 frames 2^
		}								,								
		1.5f								//reanable factor
	};



void SOneDDOParams::Mul(float v)
{
	velocity			*=	v			;
	acceleration		*=	v			;
}

void SAllDDOParams::Reset()
{
	*this=worldDisablingParams.objects_params;
}

void SAllDDOParams::Load(CInifile* ini)
{
	Reset()										;
	if(!ini)							return	;
	if(!ini->section_exist("disable"))  return	;
	if(ini->line_exist("disable","linear_factor"))	translational	.Mul(ini->r_float("disable","linear_factor"))		;
	if(ini->line_exist("disable","angular_factor"))	rotational		.Mul(ini->r_float("disable","angular_factor"))		;
	if(ini->line_exist("disable","change_count"))
	{
		int ch_cnt=ini->r_s8("disable","change_count");
		if(ch_cnt<0)L2frames=L2frames >> u16(-ch_cnt);
		else L2frames=L2frames	<< u16(ch_cnt);
		VERIFY(ch_cnt<4 && L2frames!=0 );
	}
}