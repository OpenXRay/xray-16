#pragma once
#include "alife_abstract_registry.h"

extern xr_token actor_stats_token[];


struct SStatDetailBData: public IPureSerializeObject<IReader,IWriter>
{
	shared_str				key;
	s32						int_count;
	s32						int_points;
	shared_str				str_value;

	virtual void save								(IWriter &stream);
	virtual void load								(IReader &stream);
};


typedef xr_vector<SStatDetailBData>	vStatDetailData;

struct SStatSectionData: public IPureSerializeObject<IReader,IWriter>
{
	shared_str				key;
	vStatDetailData			data;

	SStatDetailBData&		GetData			(const shared_str&);
	s32						GetTotalPoints	() const;
	virtual void			save			(IWriter &stream);
	virtual void			load			(IReader &stream);
};

typedef xr_vector<SStatSectionData> vStatSectionData;
typedef CALifeAbstractRegistry<u16, vStatSectionData> CActorStatisticRegistry;