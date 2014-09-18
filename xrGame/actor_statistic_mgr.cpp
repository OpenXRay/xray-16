#include "stdafx.h"
#include "actor_statistic_mgr.h"
#include "alife_registry_wrappers.h"
#include "alife_simulator_header.h"
/*
xr_token actor_stats_token[]={
	{"total",				100},
	{"stalkerkills",		1},
	{"monsterkills",		2},
	{"quests",				3},
	{"artefacts",			4},
	{"reputation",			5},
	{"foo",					0},
	{0,						0}
};
*/
void SStatDetailBData::save(IWriter &stream)
{
	save_data			(key,			stream);
	save_data			(int_count,		stream);
	save_data			(int_points,	stream);
	save_data			(str_value,		stream);
}

void SStatDetailBData::load(IReader &stream)
{
	load_data			(key,			stream);
	load_data			(int_count,		stream);
	load_data			(int_points,	stream);

	if(ai().get_alife()->header().version()>0x0002)
		load_data			(str_value,		stream);
}


////////////////////////////////////////////////
void SStatSectionData::save(IWriter &stream)	
{
	save_data			(data,				stream);
	save_data			(key,				stream);
};

void SStatSectionData::load(IReader &stream)	
{
	load_data			(data,				stream);
	if(ai().get_alife()->header().version()==0x0002)
	{
	int tmp;
	load_data			(tmp,				stream);
		switch(tmp)
		{
		case 100:
			key = "total";
			break;
		case 1:
			key = "stalkerkills";
			break;
		case 2:
			key = "monsterkills";
			break;
		case 3:
			key = "quests";
			break;
		case 4:
			key = "artefacts";
			break;
		case 5:
			key = "reputation";
			break;
		case 0:
			key = "foo";
			break;
		}
		s32				tmp2;
		load_data		(tmp2,				stream);// old total_points
	}else
		load_data			(key,				stream);
};

SStatDetailBData&	SStatSectionData::GetData	(const shared_str& key)
{
	vStatDetailData::iterator it		= data.begin();
	vStatDetailData::iterator it_e		= data.end();

	for(;it!=it_e;++it){
		if((*it).key == key)
			return *it;
	}
	data.resize				(data.size()+1);
	data.back				().key			= key;
	data.back				().int_count	= 0;
	data.back				().int_points	= 0;
	return data.back		();
}

s32 SStatSectionData::GetTotalPoints() const
{
	s32 res = 0;
	vStatDetailData::const_iterator it		= data.begin();
	vStatDetailData::const_iterator it_e	= data.end();
	for(;it!=it_e;++it)
	{
		if((*it).str_value.size()!=0)
			return -1;
		
		res		+= (*it).int_count*(*it).int_points;
	}
	return res;

}

CActorStatisticMgr::CActorStatisticMgr		()
{
	m_actor_stats_wrapper = xr_new<CActorStatisticsWrapper>();
	m_actor_stats_wrapper->registry().init(0);
}

CActorStatisticMgr::~CActorStatisticMgr		()
{
	xr_delete(m_actor_stats_wrapper);
}

vStatSectionData& CActorStatisticMgr::GetStorage		()
{
	return m_actor_stats_wrapper->registry().objects();
}

const vStatSectionData& CActorStatisticMgr::GetCStorage		()
{
	return m_actor_stats_wrapper->registry().objects();
}

SStatSectionData&	CActorStatisticMgr::GetSection		(const shared_str& key)
{
	vStatSectionData& d					= GetStorage();
	vStatSectionData::iterator it		= d.begin();
	vStatSectionData::iterator it_e		= d.end();
	for(;it!=it_e;++it){
		if((*it).key==key)
			return *it;
	}
	d.resize						(d.size()+1);
	d.back().key					= key;
	return	d.back					();
}
	
void CActorStatisticMgr::AddPoints(const shared_str& key, const shared_str& detail_key, const shared_str& str_value)
{
	SStatSectionData& sect		= GetSection		(key);
	SStatDetailBData& d			= sect.GetData		(detail_key);
	d.str_value					= str_value;
}

void CActorStatisticMgr::AddPoints(const shared_str& key, const shared_str& detail_key, s32 cnt, s32 pts)
{
	SStatSectionData& sect		= GetSection		(key);
	SStatDetailBData& d			= sect.GetData		(detail_key);
	d.int_count					+= cnt;
	d.int_points				+= cnt*pts;
}

s32 CActorStatisticMgr::GetSectionPoints(const shared_str& key)
{
	if( key != "total" )
		return GetSection(key).GetTotalPoints();
	else{//total
		s32 _total = -1;
		vStatSectionData& d					= GetStorage();
		vStatSectionData::iterator it		= d.begin();
		vStatSectionData::iterator it_e		= d.end();
		for(;it!=it_e;++it)
		{
			s32 _p = (*it).GetTotalPoints();

			if(_p !=-1)
			{
				if(_total==-1) _total=0;

				_total += _p;
			}
		}
		return _total;
	}
}
